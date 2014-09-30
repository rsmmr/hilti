
#include "../hilti.h"

#include "unpacker.h"
#include "codegen.h"
#include "abi.h"
#include "../builder/nodes.h"
#include "libhilti/port.h"

using namespace hilti;
using namespace hilti::codegen;

Unpacker::Unpacker(CodeGen* cg) : CGVisitor<int, UnpackArgs, UnpackResult>(cg, "codegen::Unpacker")
{
}

Unpacker::~Unpacker()
{
}

UnpackResult Unpacker::llvmUnpack(const UnpackArgs& args)
{
    auto ty = args.type;
    auto rty = ast::as<type::Reference>(ty);

    if ( rty )
        ty = rty->argType();

    assert(type::hasTrait<type::trait::Unpackable>(ty));

    auto dst = cg()->llvmAddTmp("unpack-obj", cg()->llvmType(args.type));
    auto next = cg()->llvmAddTmp("unpack-next", cg()->llvmLibType("hlt.iterator.bytes"));
    UnpackResult result;
    result.value_ptr = dst;
    result.iter_ptr = next;

    setArg1(args);
    setArg2(result);
    call(ty);

    return result;
}

void Unpacker::visit(type::Reference* t)
{
    call(t->argType());
}

static void _bytesUnpackFixed(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool skip, bool eod_ok)
{
    auto iter_type = builder::iterator::typeBytes();
    auto begin = builder::codegen::create(iter_type, args.begin);
    auto llvm_arg = cg->builder()->CreateZExt(args.arg, cg->llvmTypeInt(64));
    auto arg = builder::codegen::create(builder::integer::type(64), llvm_arg);

    // Get end position.
    CodeGen::expr_list params = { begin, arg };
    auto llvm_next = cg->llvmCall("hlt::iterator_bytes_incr_by", params);

    // Check if reached end of bytes object.
    auto next = builder::codegen::create(iter_type, llvm_next);
    auto end = cg->llvmConstBytesEnd();

    params = { next, builder::codegen::create(iter_type, end) };
    auto is_end = cg->llvmCall("hlt::iterator_bytes_eq", params );

    auto block_end_size = cg->newBuilder("unpack-end-check-size");
    auto block_end_eod = cg->newBuilder("unpack-end-check-eod");
    auto block_done = cg->newBuilder("unpack-done");
    auto block_insufficient = cg->newBuilder("unpack-out-of-input");
    cg->llvmCreateCondBr(is_end, block_end_size, block_done);

    // When we've reached the end, we must check whether we got enough bytes
    // or not if eod is not ok or the object isn't frozen.
    cg->pushBuilder(block_end_size);

    params = { begin, next };
    auto diff = cg->llvmCall("hlt::iterator_bytes_diff", params);
    auto len = cg->builder()->CreateZExt(diff, cg->llvmTypeInt(64));
    auto enough = cg->builder()->CreateICmpULT(len, llvm_arg);
    cg->llvmCreateCondBr(enough, block_end_eod, block_done);

    cg->pushBuilder(block_end_eod);

    if ( eod_ok ) {
        params = { begin };
        auto frozen = cg->llvmCall("hlt::iterator_bytes_is_frozen", params);
        cg->llvmCreateCondBr(frozen, block_done, block_insufficient);
    }

    else
        cg->llvmCreateBr(block_insufficient);

    cg->popBuilder();

    cg->popBuilder();

    // Not enough input.
    cg->pushBuilder(block_insufficient);
    cg->llvmRaiseException("Hilti::WouldBlock", args.location);
    cg->popBuilder();

    // Got it, extract the bytes.
    cg->pushBuilder(block_done);

    llvm::Value* val = 0;

    if ( ! skip ) {
        params = { begin, next };
        val = cg->llvmCall("hlt::bytes_sub", params);
    }
    else
        val = cg->llvmConstNull();

    cg->llvmCreateStore(val, result.value_ptr);
    cg->llvmCreateStore(llvm_next, result.iter_ptr);

    // Leave builder on stack.
}

static void _bytesUnpackRunLength(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool skip)
{
    auto iter_type = builder::iterator::typeBytes();

    auto len = cg->llvmUnpack(builder::integer::type(64), args.begin, args.end, args.arg, nullptr, nullptr, args.location);

    UnpackArgs nargs = args;
    nargs.arg = len.first;
    nargs.begin = len.second;

    _bytesUnpackFixed(cg, nargs, result, skip, false);
}

static void _bytesUnpackDelim(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool skip)
{
    auto iter_type = builder::iterator::typeBytes();
    auto bytes_type = builder::reference::type(builder::bytes::type());
    auto delim = builder::codegen::create(bytes_type, args.arg);
    auto begin = builder::codegen::create(iter_type, args.begin);
    auto end = builder::codegen::create(iter_type, cg->llvmIterBytesEnd());

    auto block_body = cg->newBuilder("unpack-loop-body");
    auto block_cmp = cg->newBuilder("unpack-loop-cmp");
    auto block_exit = cg->newBuilder("unpack-loop-exit");
    auto block_next = cg->newBuilder("unpack-loop-next");
    auto block_insufficient = cg->newBuilder("unpack-loop-insufficient");

    // Copy the start iterator.
    cg->llvmCreateStore(args.begin, result.iter_ptr);

    // Enter loop.
    cg->llvmCreateBr(block_body);

    // Check whether end reached.
    cg->pushBuilder(block_body);
    auto cur = builder::codegen::create(iter_type, cg->builder()->CreateLoad(result.iter_ptr));
    CodeGen::expr_list params = { cur, end };
    auto done = cg->llvmCall("hlt::iterator_bytes_eq", params);
    cg->llvmCreateCondBr(done, block_insufficient, block_cmp);
    cg->popBuilder();

    // Check whether we found the delimiter.
    cg->pushBuilder(block_cmp);
    cur = builder::codegen::create(iter_type, cg->builder()->CreateLoad(result.iter_ptr));
    params = { cur, delim };
    auto found = cg->llvmCall("hlt::bytes_match_at", params);
    cg->llvmCreateCondBr(found, block_exit, block_next);
    cg->popBuilder();

    // Move iterator forward.
    cg->pushBuilder(block_next);
    cur = builder::codegen::create(iter_type, cg->builder()->CreateLoad(result.iter_ptr));
    params = { cur };
    auto next = cg->llvmCall("hlt::iterator_bytes_incr", params);
    cg->llvmCreateStore(next, result.iter_ptr);
    cg->llvmCreateBr(block_body);
    cg->popBuilder();

    // Not found.
    cg->pushBuilder(block_insufficient);
    cg->llvmRaiseException("Hilti::WouldBlock", args.location);
    cg->popBuilder();

    // Loop exit.
    cg->pushBuilder(block_exit);

    if ( ! skip ) {
        // Get the string up to here.
        cur = builder::codegen::create(iter_type, cg->builder()->CreateLoad(result.iter_ptr));
        params = { begin, cur };
        auto match = cg->llvmCall("hlt::bytes_sub", params);
        cg->llvmCreateStore(match, result.value_ptr);
    }

    else {
        auto init_val = cg->llvmInitVal(bytes_type);
        cg->llvmCreateStore(init_val, result.value_ptr);
    }

    // Move beyond delimiter.
    params = { delim };
    auto l = cg->llvmCall("hlt::bytes_len", params);
    auto len = builder::codegen::create(builder::integer::type(64), l);
    cur = builder::codegen::create(iter_type, cg->builder()->CreateLoad(result.iter_ptr));
    params = { cur, len };
    next = cg->llvmCall("hlt::iterator_bytes_incr_by", params);
    cg->llvmCreateStore(next, result.iter_ptr);

    // Leave builder on stack.
}

void Unpacker::visit(type::Bytes* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "bytes-run-length",
        cg()->llvmEnum("Hilti::Packed::BytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed",
        cg()->llvmEnum("Hilti::Packed::BytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, false, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed-or-eod",
        cg()->llvmEnum("Hilti::Packed::BytesFixedOrEod"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, false, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::BytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-run-length-skip",
        cg()->llvmEnum("Hilti::Packed::SkipBytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed-skip",
        cg()->llvmEnum("Hilti::Packed::SkipBytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, true, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed-skip-or-eod",
        cg()->llvmEnum("Hilti::Packed::SkipBytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, true, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim-skip",
        cg()->llvmEnum("Hilti::Packed::SkipBytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(cg, args, result, true); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static llvm::Value* _castToWidth(CodeGen* cg, llvm::Value* val, int twidth, bool sign)
{
    int width = llvm::cast<llvm::IntegerType>(val->getType())->getBitWidth();
    auto ttype = cg->llvmTypeInt(twidth);

    if ( width < twidth ) {
        if ( sign )
            val = cg->builder()->CreateSExt(val, ttype);
        else
            val = cg->builder()->CreateZExt(val, ttype);
    }

    if ( width > twidth )
        val = cg->builder()->CreateTrunc(val, ttype);

    return val;
}

static void _integerUnpack(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, int width, bool sign, ABI::ByteOrder order, const std::list<int> bytes)
{
    auto iter_type = builder::iterator::typeBytes();
    auto twidth = ast::as<type::Integer>(args.type)->width();
    auto itype = cg->llvmTypeInt(width);

    // Copy the start iterator.
    cg->llvmCreateStore(args.begin, result.iter_ptr);

    llvm::Value* unpacked = cg->llvmConstNull(itype);

    // Extract the bytes.
    for ( auto i : bytes ) {
        llvm::Value* byte = cg->llvmCallC("__hlt_bytes_extract_one", { result.iter_ptr, args.end }, true);

        byte = cg->builder()->CreateZExt(byte, itype);

        if ( i )
            byte = cg->builder()->CreateShl(byte, cg->llvmConstInt(i * 8, width));

        unpacked = cg->builder()->CreateOr(unpacked, byte);
    }

    unpacked = _castToWidth(cg, unpacked, twidth, sign);

    // Select subset of bits if requested.
    if ( args.arg ) {
        auto low = cg->llvmTupleElement(args.arg_type, args.arg, 0, false);
        auto high = cg->llvmTupleElement(args.arg_type, args.arg, 1, false);

        low = _castToWidth(cg, low, twidth, sign);
        high = _castToWidth(cg, high, twidth, sign);

        unpacked = cg->llvmExtractBits(unpacked, low, high);
    }

    cg->llvmCreateStore(unpacked, result.value_ptr);
}

void Unpacker::visit(type::Integer* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    std::list<llvm::Value*> i8l = { cg()->llvmEnum("Hilti::Packed::Int8Little") };
    std::list<llvm::Value*> i16l = { cg()->llvmEnum("Hilti::Packed::Int16Little") };
    std::list<llvm::Value*> i32l = { cg()->llvmEnum("Hilti::Packed::Int32Little") };
    std::list<llvm::Value*> i64l = { cg()->llvmEnum("Hilti::Packed::Int64Little") };

    std::list<llvm::Value*> i8b = { cg()->llvmEnum("Hilti::Packed::Int8Big") };
    std::list<llvm::Value*> i16b = { cg()->llvmEnum("Hilti::Packed::Int16Big") };
    std::list<llvm::Value*> i32b = { cg()->llvmEnum("Hilti::Packed::Int32Big") };
    std::list<llvm::Value*> i64b = { cg()->llvmEnum("Hilti::Packed::Int64Big") };

    std::list<llvm::Value*> u8l = { cg()->llvmEnum("Hilti::Packed::UInt8Little") };
    std::list<llvm::Value*> u16l = { cg()->llvmEnum("Hilti::Packed::UInt16Little") };
    std::list<llvm::Value*> u32l = { cg()->llvmEnum("Hilti::Packed::UInt32Little") };
    std::list<llvm::Value*> u64l = { cg()->llvmEnum("Hilti::Packed::UInt64Little") };

    std::list<llvm::Value*> u8b = { cg()->llvmEnum("Hilti::Packed::UInt8Big") };
    std::list<llvm::Value*> u16b = { cg()->llvmEnum("Hilti::Packed::UInt16Big") };
    std::list<llvm::Value*> u32b = { cg()->llvmEnum("Hilti::Packed::UInt32Big") };
    std::list<llvm::Value*> u64b = { cg()->llvmEnum("Hilti::Packed::UInt64Big") };

    switch ( cg()->abi()->byteOrder() ) {
     case ABI::LittleEndian:
        i8l.push_back(cg()->llvmEnum("Hilti::Packed::Int8"));
        i16l.push_back(cg()->llvmEnum("Hilti::Packed::Int16"));
        i32l.push_back(cg()->llvmEnum("Hilti::Packed::Int32"));
        i64l.push_back(cg()->llvmEnum("Hilti::Packed::Int64"));

        u8l.push_back(cg()->llvmEnum("Hilti::Packed::UInt8"));
        u16l.push_back(cg()->llvmEnum("Hilti::Packed::UInt16"));
        u32l.push_back(cg()->llvmEnum("Hilti::Packed::UInt32"));
        u64l.push_back(cg()->llvmEnum("Hilti::Packed::UInt64"));
        break;

     case ABI::BigEndian:
        i8b.push_back(cg()->llvmEnum("Hilti::Packed::Int8"));
        i16b.push_back(cg()->llvmEnum("Hilti::Packed::Int16"));
        i32b.push_back(cg()->llvmEnum("Hilti::Packed::Int32"));
        i64b.push_back(cg()->llvmEnum("Hilti::Packed::Int64"));

        u8b.push_back(cg()->llvmEnum("Hilti::Packed::UInt8"));
        u16b.push_back(cg()->llvmEnum("Hilti::Packed::UInt16"));
        u32b.push_back(cg()->llvmEnum("Hilti::Packed::UInt32"));
        u64b.push_back(cg()->llvmEnum("Hilti::Packed::UInt64"));
        break;

     default:
        internalError("unknown byte order");
    }

    cases.push_back(CodeGen::SwitchCase(
        "int8l", i8l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 8, true, ABI::LittleEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int16l", i16l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 16, true, ABI::LittleEndian, {0, 1}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int32l", i32l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 32, true, ABI::LittleEndian, {0, 1, 2, 3}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int64l", i64l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 64, true, ABI::LittleEndian, {0, 1, 2, 3, 4, 5, 6, 7}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "int8b", i8b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 8, true, ABI::BigEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int16b", i16b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 16, true, ABI::BigEndian, {1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int32b", i32b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 32, true, ABI::BigEndian, {3, 2, 1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int64l", i64b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 64, true, ABI::BigEndian, {7, 6, 5, 4, 3, 2, 1, 0}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "uint8l", u8l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 8, false, ABI::LittleEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint16l", u16l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 16, false, ABI::LittleEndian, {0, 1}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint32l", u32l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 32, false, ABI::LittleEndian, {0, 1, 2, 3}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint64l", u64l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 64, false, ABI::LittleEndian, {0, 1, 2, 3, 4, 5, 6, 7}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "uint8b", u8b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 8, false, ABI::BigEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint16b", u16b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 16, false, ABI::BigEndian, {1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint32b", u32b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 32, false, ABI::BigEndian, {3, 2, 1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint64l", u64b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerUnpack(cg, args, result, 64, false, ABI::BigEndian, {7, 6, 5, 4, 3, 2, 1, 0}); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static void _addrUnpack4(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool nbo)
{
    auto iter_type = builder::iterator::typeBytes();

    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int32Big" : "Hilti::Packed::Int32Little");
    auto a = cg->llvmConstInt(0, 64);
    auto b = cg->llvmUnpack(builder::integer::type(32), args.begin, args.end, fmt, nullptr, nullptr, args.location);

    CodeGen::value_list vals = { a, cg->builder()->CreateZExt(b.first, cg->llvmTypeInt(64)) };
    auto addr = cg->llvmValueStruct(vals);

    cg->llvmCreateStore(addr, result.value_ptr);
    cg->llvmCreateStore(b.second, result.iter_ptr);
}

static void _addrUnpack6(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool nbo)
{
    auto iter_type = builder::iterator::typeBytes();

    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int64Big" : "Hilti::Packed::Int64Little");
    auto a = cg->llvmUnpack(builder::integer::type(64), args.begin, args.end, fmt, nullptr, nullptr, args.location);
    auto b = cg->llvmUnpack(builder::integer::type(64), a.second, args.end, fmt, nullptr, nullptr, args.location);

    CodeGen::value_list vals;

    if ( ! nbo )
        vals = { b.first, a.first };
    else
        vals = { a.first, b.first };

    auto addr = cg->llvmValueStruct(vals);

    cg->llvmCreateStore(addr, result.value_ptr);
    cg->llvmCreateStore(b.second, result.iter_ptr);
}

void Unpacker::visit(type::Address* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    bool host_is_nbo = (cg()->abi()->byteOrder() == ABI::BigEndian);

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4",
        cg()->llvmEnum("Hilti::Packed::IPv4"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack4(cg, args, result, host_is_nbo); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-network",
        cg()->llvmEnum("Hilti::Packed::IPv4Network"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack4(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-little",
        cg()->llvmEnum("Hilti::Packed::IPv4Little"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack4(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-big",
        cg()->llvmEnum("Hilti::Packed::IPv4Big"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack4(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6",
        cg()->llvmEnum("Hilti::Packed::IPv6"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack6(cg, args, result, host_is_nbo); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-network",
        cg()->llvmEnum("Hilti::Packed::IPv6Network"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack6(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-little",
        cg()->llvmEnum("Hilti::Packed::IPv6Little"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack6(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-big",
        cg()->llvmEnum("Hilti::Packed::IPv6Big"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrUnpack6(cg, args, result, true); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static void _portUnpack(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool nbo, bool tcp)
{
    auto iter_type = builder::iterator::typeBytes();

    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int16Big" : "Hilti::Packed::Int16");
    auto port = cg->llvmUnpack(builder::integer::type(16), args.begin, args.end, fmt, nullptr, nullptr, args.location);
    auto proto = tcp ? HLT_PORT_TCP : HLT_PORT_UDP;

    CodeGen::value_list vals = { port.first, cg->llvmConstInt(proto, 8) };
    auto addr = cg->llvmValueStruct(vals, true);

    cg->llvmCreateStore(addr, result.value_ptr);
    cg->llvmCreateStore(port.second, result.iter_ptr);
}

void Unpacker::visit(type::Port* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "port-tcp",
        cg()->llvmEnum("Hilti::Packed::PortTCP"),
        [&] (CodeGen* cg) -> llvm::Value* { _portUnpack(cg, args, result, false, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-tcp-network",
        cg()->llvmEnum("Hilti::Packed::PortTCPNetwork"),
        [&] (CodeGen* cg) -> llvm::Value* { _portUnpack(cg, args, result, true, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-udp",
        cg()->llvmEnum("Hilti::Packed::PortUDP"),
        [&] (CodeGen* cg) -> llvm::Value* { _portUnpack(cg, args, result, false, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-udp-network",
        cg()->llvmEnum("Hilti::Packed::PortUDPNetwork"),
        [&] (CodeGen* cg) -> llvm::Value* { _portUnpack(cg, args, result, true, false); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

void Unpacker::visit(type::Bool* t)
{
    auto iter_type = builder::iterator::typeBytes();

    auto args = arg1();
    auto result = arg2();

    // Format can only Hilti::Packed::Bool.

    // Copy the start iterator.
    cg()->llvmCreateStore(args.begin, result.iter_ptr);

    llvm::Value* value = cg()->llvmCallC("__hlt_bytes_extract_one", { result.iter_ptr, args.end }, true);

    // Select subset of bits if requested.
    if ( args.arg ) {
        auto bit = cg()->builder()->CreateTrunc(args.arg, cg()->llvmTypeInt(8));
        value = cg()->llvmExtractBits(value, bit, bit);
    }

    auto unpacked = cg()->builder()->CreateICmpNE(value, cg()->llvmConstInt(0, 8));

    cg()->llvmCreateStore(unpacked, result.value_ptr);
}

