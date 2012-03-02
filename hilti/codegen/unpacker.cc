
#include "../hilti.h"

#include "unpacker.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Unpacker::Unpacker(CodeGen* cg) : CGVisitor<UnpackResult, UnpackArgs>(cg, "codegen::Unpacker")
{
}

Unpacker::~Unpacker()
{
}

UnpackResult Unpacker::llvmUnpack(const UnpackArgs& args)
{
    assert(type::hasTrait<type::trait::Unpackable>(args.type));

    setArg1(args);
    call(args.type);

    UnpackResult result;
    bool success = processOne(args.type, &result);
    assert(success);

    return result;
}

void Unpacker::visit(type::Reference* t)
{
    call(t->refType());
}

static void _bytesUnpackRunLength(const UnpackArgs& args, llvm::Value* dst, llvm::Value* next, bool skip)
{
}

static void _bytesUnpackFixed(const UnpackArgs& args, llvm::Value* dst, llvm::Value* next, bool skip)
{
}

static void _bytesUnpackDelim(const UnpackArgs& args, llvm::Value* dst, llvm::Value* next, bool skip)
{
}

void Unpacker::visit(type::Bytes* t)
{
    auto args = arg1();
    auto dst = cg()->llvmAddTmp("unpack-dst", cg()->llvmLibType("hlt.bytes"));
    auto next = cg()->llvmAddTmp("unpack-next", cg()->llvmLibType("hlt.iterator.bytes"));

    std::list<llvm::ConstantInt*> a;

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::BytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(args, dst, next, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::BytesFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(args, dst, next, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::BytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(args, dst, next, false); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::SkipBytesRunLength"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackRunLength(args, dst, next, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "run-length",
        cg()->llvmEnum("Hilti::Packed::SkipBytesRunFixed"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackFixed(args, dst, next, true); return nullptr; }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "bytes-delim",
        cg()->llvmEnum("Hilti::Packed::SkipBytesDelim"),
        [&] (CodeGen* cg) -> llvm::Value* { _bytesUnpackDelim(args, dst, next, true); return nullptr; }
    ));

    cg()->llvmSwitch(args.fmt, cases);

    setResult(dst, next);
}

//            ("Hilti::Packed::SkipBytesRunLength", unpackRunLength(arg, True)),
//          ("Hilti::Packed::SkipBytesFixed", unpackFixed(arg, True, begin)),
//            ("Hilti::Packed::SkipBytesDelim", unpackDelim(arg, True)),
//            ]

