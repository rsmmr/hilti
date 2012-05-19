
#include "../hilti.h"

#include "coercer.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

codegen::Coercer::Coercer(CodeGen* cg) : CGVisitor<llvm::Value*, llvm::Value*, shared_ptr<hilti::Type>>(cg, "codegen::Coercer")
{
}

codegen::Coercer::~Coercer()
{
}

llvm::Value* codegen::Coercer::llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src, shared_ptr<hilti::Type> dst)
{
    if ( src->equal(dst) || dst->equal(src) )
        return value;

    if ( ast::isA<type::OptionalArgument>(dst) )
        return llvmCoerceTo(value, src, ast::as<type::OptionalArgument>(dst)->argType());

    setArg1(value);
    setArg2(dst);

    llvm::Value* result;
    bool success = processOne(src, &result);
    assert(success);
    return result;
}

void codegen::Coercer::visit(type::Integer* t)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Bool> dst_b = ast::as<type::Bool>(dst);

    if ( dst_b ) {
        auto width = llvm::cast<llvm::IntegerType>(val->getType())->getBitWidth();
        auto result = builder()->CreateICmpEQ(val, cg()->llvmConstInt(0, width));
        setResult(result);
    }

    assert(false);
}

void codegen::Coercer::visit(type::Tuple* t)
{
    auto val = arg1();
    auto dst = arg2();

    assert(false);

    setResult(val);

}

void codegen::Coercer::visit(type::Address* t)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Network> dst_net = ast::as<type::Network>(dst);

    if ( dst_net ) {
        auto a = cg()->llvmExtractValue(val, 0);
        auto b = cg()->llvmExtractValue(val, 1);
        auto w = cg()->llvmConstInt(128, 8);

        CodeGen::value_list vals = { a, b, w };
        auto net = cg()->llvmValueStruct(vals);

        setResult(net);
        return;
    }

    assert(false);
}
