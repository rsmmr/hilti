
#include "../hilti.h"

#include "unpacker.h"
#include "codegen.h"
#include "builder.h"

using namespace hilti;
using namespace codegen;

Unpacker::Unpacker(CodeGen* cg) : CGVisitor<int, UnpackArgs, UnpackResult>(cg, "codegen::Unpacker")
{
}

Unpacker::~Unpacker()
{
}

UnpackResult Unpacker::llvmUnpack(const UnpackArgs& args)
{
    assert(type::hasTrait<type::trait::Unpackable>(args.type));

    auto dst = cg()->llvmAddTmp("unpack-obj", cg()->llvmLibType("hlt.bytes"));
    auto next = cg()->llvmAddTmp("unpack-next", cg()->llvmLibType("hlt.iterator.bytes"));
    UnpackResult result;
    result.value_ptr = dst;
    result.iter_ptr = next;

    setArg1(args);
    setArg2(result);
    call(args.type);

    bool success = processOne(args.type, args, result);
    assert(success);

    return result;
}

void Unpacker::visit(type::Reference* t)
{
    call(t->argType());
}

static void _bytesUnpackFixed(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool skip)
{
    auto iter_type = builder::iterator::typeBytes();
    auto begin = builder::codegen::create(builder::iterator::typeBytes(), args.begin);
    auto llvm_arg = cg->builder()->CreateZExt(args.arg, cg->llvmTypeInt(64));
    auto arg = builder::codegen::create(builder::integer::type(64), llvm_arg);

    // Get end position.
    CodeGen::expr_list params;
    params.push_back(begin);
    params.push_back(arg);
    auto llvm_next = cg->llvmCall("hlt::bytes_pos_incr_by", params);
    auto next = builder::codegen::create(iter_type, llvm_next);

    // Check if reached end of bytes object.
    params.clear();
    params.push_back(next);
    params.push_back(builder::codegen::create(iter_type, cg->llvmConstBytesEnd()));
    auto is_end = cg->llvmCall("hlt::bytes_pos_eq", params);

    auto block_end = cg->newBuilder("unpack-end");
    auto block_done = cg->newBuilder("unpack-done");
    auto block_insufficient = cg->newBuilder("unpack-out-of-input");
    cg->llvmCreateCondBr(is_end, block_end, block_done);

    // When we've reached the end, we must check whether we got enough bytes
    // or not.
    cg->pushBuilder(block_end);
    params.clear();
    params.push_back(begin);
    params.push_back(next);
    auto diff = cg->llvmCall("hlt::bytes_pos_diff", params);
    auto len = cg->builder()->CreateZExt(diff, cg->llvmTypeInt(64));
    auto enough = cg->builder()->CreateICmpULT(len, llvm_arg);
    cg->llvmCreateCondBr(enough, block_insufficient, block_done);

    cg->popBuilder();

    // Not enough input.
    cg->pushBuilder(block_insufficient);
    cg->llvmRaiseException("hlt_exception_would_block", args.location);
    cg->popBuilder();

    // Got it, extract the bytes.
    cg->pushBuilder(block_done);

    llvm::Value* val = 0;

    if ( ! skip ) {
        params.clear();
        params.push_back(begin);
        params.push_back(next);
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
    auto len = cg->llvmUnpack(builder::integer::type(64), args.begin, args.end, args.arg, nullptr, args.location);

    UnpackArgs nargs = args;
    nargs.arg = len.first;
    nargs.begin = len.second;

    _bytesUnpackFixed(cg, nargs, result, skip);
}

static void _bytesUnpackDelim(CodeGen* cg, const UnpackArgs& args, const UnpackResult& result, bool skip)
{
#if 0
                if delim and len(delim) == 1:
                    # We optimize for the case of a single, constant byte.

                    delim = cg.llvmConstInt(ord(delim), 8)
                    block_body = cg.llvmNewBlock("loop-body-start")
                    block_cmp = cg.llvmNewBlock("loop-body-cmp")
                    block_exit = cg.llvmNewBlock("loop-exit")
                    block_insufficient = cg.llvmNewBlock("unpack-out-of-input")

                    # Copy the start iterator.
                    builder = cg.builder()
                    builder.store(begin, iter)

                    # Enter loop.
                    builder.branch(block_body)

                    # Loop body.

                        # Check whether end reached.
                    builder = cg.pushBuilder(block_body)
                    arg1 = operand.LLVM(builder.load(iter), type.IteratorBytes())
                    arg2 = operand.LLVM(end, type.IteratorBytes())
                    done = cg.llvmCallC("hlt::bytes_pos_eq", [arg1, arg2])
                    builder = cg.builder()
                    builder.cbranch(done, block_insufficient, block_cmp)
                    cg.popBuilder()

                        # Check whether we found the delimiter
                    builder = cg.pushBuilder(block_cmp)
                    byte = cg.llvmCallCInternal("__hlt_bytes_extract_one", [iter, end, exception, ctx])
                    done = builder.icmp(llvm.core.IPRED_EQ, byte, delim)
                    builder.cbranch(done, block_exit, block_body)
                    cg.popBuilder()

                        # Not found.
                    builder = cg.pushBuilder(block_insufficient)
                    cg.llvmRaiseExceptionByName("hlt_exception_would_block", self.location())
                    cg.popBuilder()

                    # Loop exit.
                    builder = cg.pushBuilder(block_exit)

                    if not skip:
                        arg1 = operand.LLVM(begin, type.IteratorBytes())
                        arg2 = operand.LLVM(builder.load(iter), type.IteratorBytes())
                        val = cg.llvmCallC("hlt::bytes_sub", [arg1, arg2])
                    else:
                        val = llvm.core.Constant.null(cg.llvmTypeGenericPointer())

                    cg.llvmAssign(val, bytes)

                # Leave builder on stack.

                else:
                    # Everything else we outsource to C.
                    # FIXME: Well, not yet ...
                    util.internal_error("byte unpacking with delimiters only supported for a single constant byte yet")    
#endif
}

void Unpacker::visit(type::Bytes* t)
{
    auto args = arg1();
    auto result = arg2();

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::BytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::BytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::BytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(cg, args, result, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::SkipBytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::SkipBytesRunFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(cg, args, result, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::SkipBytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(cg, args, result, true); return nullptr; }
    ));

    cg()->llvmSwitchEnumConst(args.fmt, cases);
}

void Unpacker::visit(type::Integer* t)
{
    
}

