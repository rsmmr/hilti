
#include "../hilti.h"

#include "packer.h"
#include "codegen.h"
#include "abi.h"
#include "../builder/nodes.h"
#include "libhilti/port.h"

using namespace hilti;
using namespace hilti::codegen;

Packer::Packer(CodeGen* cg) : CGVisitor<int, PackArgs, PackResult>(cg, "codegen::Packer")
{
}

Packer::~Packer()
{
}

PackResult Packer::llvmPack(const PackArgs& args)
{
    auto ty = args.type;
    auto rty = ast::as<type::Reference>(ty);

    if ( rty )
        ty = rty->argType();

    assert(type::hasTrait<type::trait::Unpackable>(ty));

    auto btype = builder::reference::type(builder::bytes::type());
    auto result = cg()->llvmAddTmp("packed", cg()->llvmType(btype));

    setArg1(args);
    setArg2(result);
    call(ty);

    return cg()->builder()->CreateLoad(result);
}

void Packer::visit(type::Reference* t)
{
    call(t->argType());
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

static void _bytesPackRunLength(CodeGen* cg, const PackArgs& args, const PackResult& result)
{
    auto b = std::make_shared<expression::CodeGen>(args.type, args.value);

    auto len = cg->llvmCall("hlt::bytes_len", { b });
    auto packed = cg->llvmPack(len, builder::integer::type(64), args.arg, nullptr, nullptr, args.location);

    cg->llvmCallC("__hlt_bytes_append", { packed, args.value }, true);
    cg->llvmCreateStore(packed, result);
}

static void _bytesPackDelim(CodeGen* cg, const PackArgs& args, const PackResult& result)
{
    auto b1 = std::make_shared<expression::CodeGen>(args.type, args.value);
    auto b2 = std::make_shared<expression::CodeGen>(args.arg_type, args.arg);

    auto packed = cg->llvmCall("hlt::bytes_concat", { b1, b2 });
    cg->llvmCreateStore(packed, result);
}

void Packer::visit(type::Bytes* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "bytes-run-length",
        cg()->llvmEnum("Hilti::Packed::BytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesPackRunLength(cg, args, result); return nullptr; }
    ));

#if 0
    // Not useful for packing.
    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed",
        cg()->llvmEnum("Hilti::Packed::BytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesPackFixed(cg, args, result); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-fixed-or-eod",
        cg()->llvmEnum("Hilti::Packed::BytesFixedOrEod"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesPackFixed(cg, args, result); return nullptr; }
    ));
#endif

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::BytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesPackDelim(cg, args, result); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static void _integerPack(CodeGen* cg, const PackArgs& args, const PackResult& result, int width, bool sign, ABI::ByteOrder order, const std::list<int> bytes)
{
    llvm::Value* nval = args.value;
    nval = _castToWidth(cg, nval, width, false);

    auto twidth = ast::as<type::Integer>(args.type)->width();

    // Select subset of bits if requested.
    if ( args.arg ) {
        auto low = cg->llvmTupleElement(args.arg_type, args.arg, 0, false);
        auto high = cg->llvmTupleElement(args.arg_type, args.arg, 1, false);

        low = _castToWidth(cg, low, twidth, sign);
        high = _castToWidth(cg, high, twidth, sign);

        nval = cg->llvmInsertBits(nval, low, high);
    }

    auto packed = cg->llvmCall("hlt::bytes_new", {});
    auto ch = cg->llvmAddTmp("ch", cg->llvmTypeInt(8));
    auto one = cg->llvmConstInt(1, 64);

    for ( auto i : bytes ) {

        cg->llvmCreateStore(cg->llvmConstInt(i, 8), ch);

        auto val = nval;
        if ( sign )
            val = cg->builder()->CreateAShr(nval, cg->llvmConstInt(i * 8, width));
        else
            val = cg->builder()->CreateLShr(nval, cg->llvmConstInt(i * 8, width));

        val = cg->builder()->CreateTrunc(val, cg->llvmTypeInt(8));

        cg->builder()->CreateStore(val, ch);
        cg->llvmCallC("hlt_bytes_append_raw_copy", { packed, ch, one }, true);
    }

    cg->llvmCreateStore(packed, result);
}

void Packer::visit(type::Integer* t)
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
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 8, true, ABI::LittleEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int16l", i16l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 16, true, ABI::LittleEndian, {0, 1}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int32l", i32l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 32, true, ABI::LittleEndian, {0, 1, 2, 3}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int64l", i64l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 64, true, ABI::LittleEndian, {0, 1, 2, 3, 4, 5, 6, 7}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "int8b", i8b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 8, true, ABI::BigEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int16b", i16b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 16, true, ABI::BigEndian, {1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int32b", i32b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 32, true, ABI::BigEndian, {3, 2, 1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "int64l", i64b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 64, true, ABI::BigEndian, {7, 6, 5, 4, 3, 2, 1, 0}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "uint8l", u8l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 8, false, ABI::LittleEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint16l", u16l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 16, false, ABI::LittleEndian, {0, 1}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint32l", u32l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 32, false, ABI::LittleEndian, {0, 1, 2, 3}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint64l", u64l,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 64, false, ABI::LittleEndian, {0, 1, 2, 3, 4, 5, 6, 7}); return nullptr; }
    ));

    ///

    cases.push_back(CodeGen::SwitchCase(
        "uint8b", u8b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 8, false, ABI::BigEndian, {0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint16b", u16b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 16, false, ABI::BigEndian, {1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint32b", u32b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 32, false, ABI::BigEndian, {3, 2, 1, 0}); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "uint64b", u64b,
        [&] (CodeGen* cg) -> llvm::Value* { _integerPack(cg, args, result, 64, false, ABI::BigEndian, {7, 6, 5, 4, 3, 2, 1, 0}); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static void _addrPack4(CodeGen* cg, const PackArgs& args, const PackResult& result, bool nbo)
{
    auto b = cg->llvmExtractValue(args.value, 1);
    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int32Big" : "Hilti::Packed::Int32Little");

    auto tmp = cg->llvmPack(b, builder::integer::type(32), fmt, nullptr, nullptr, args.location);
    cg->builder()->CreateStore(tmp, result);
}

static void _addrPack6(CodeGen* cg, const PackArgs& args, const PackResult& result, bool nbo)
{
    auto a = cg->llvmExtractValue(args.value, 0);
    auto b = cg->llvmExtractValue(args.value, 1);
    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int64Big" : "Hilti::Packed::Int64Little");

    auto tmp1 = cg->llvmPack(a, builder::integer::type(64), fmt, nullptr, nullptr, args.location);
    auto tmp2 = cg->llvmPack(b, builder::integer::type(64), fmt, nullptr, nullptr, args.location);

    if ( nbo ) {
        cg->llvmCallC("__hlt_bytes_append", { tmp1, tmp2 }, true);
        cg->llvmCreateStore(tmp1, result);
    }

    else {
        cg->llvmCallC("__hlt_bytes_append", { tmp2, tmp1 }, true);
        cg->llvmCreateStore(tmp2, result);
    }
}

void Packer::visit(type::Address* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    bool host_is_nbo = (cg()->abi()->byteOrder() == ABI::BigEndian);

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4",
        cg()->llvmEnum("Hilti::Packed::IPv4"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack4(cg, args, result, host_is_nbo); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-network",
        cg()->llvmEnum("Hilti::Packed::IPv4Network"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack4(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-little",
        cg()->llvmEnum("Hilti::Packed::IPv4Little"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack4(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv4-big",
        cg()->llvmEnum("Hilti::Packed::IPv4Big"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack4(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6",
        cg()->llvmEnum("Hilti::Packed::IPv6"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack6(cg, args, result, host_is_nbo); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-network",
        cg()->llvmEnum("Hilti::Packed::IPv6Network"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack6(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-little",
        cg()->llvmEnum("Hilti::Packed::IPv6Little"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack6(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "addr-ipv6-big",
        cg()->llvmEnum("Hilti::Packed::IPv6Big"),
        [&] (CodeGen* cg) -> llvm::Value* { _addrPack6(cg, args, result, true); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

static void _portPack(CodeGen* cg, const PackArgs& args, const PackResult& result, bool nbo, bool tcp)
{
    auto fmt = cg->llvmEnum(nbo ? "Hilti::Packed::Int16Big" : "Hilti::Packed::Int16");

    auto port = cg->llvmExtractValue(args.value, 0);
    // auto proto = cg->llvmExtractValue(args.value, 1);

    auto tmp = cg->llvmPack(port, builder::integer::type(16), fmt, nullptr, nullptr, args.location);
    cg->builder()->CreateStore(tmp, result);
}

void Packer::visit(type::Port* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "port-tcp",
        cg()->llvmEnum("Hilti::Packed::PortTCP"),
        [&] (CodeGen* cg) -> llvm::Value* { _portPack(cg, args, result, false, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-tcp-network",
        cg()->llvmEnum("Hilti::Packed::PortTCPNetwork"),
        [&] (CodeGen* cg) -> llvm::Value* { _portPack(cg, args, result, true, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-udp",
        cg()->llvmEnum("Hilti::Packed::PortUDP"),
        [&] (CodeGen* cg) -> llvm::Value* { _portPack(cg, args, result, false, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "port-udp-network",
        cg()->llvmEnum("Hilti::Packed::PortUDPNetwork"),
        [&] (CodeGen* cg) -> llvm::Value* { _portPack(cg, args, result, true, false); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

void Packer::visit(type::Bool* t)
{
}

