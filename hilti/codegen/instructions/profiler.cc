
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"
#include "hilti/autogen/instructions.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::profiler::Start* i)
{
    llvm::Value* tag = cg()->llvmValue(i->op1());
    llvm::Value* style = nullptr;
    llvm::Value* param = nullptr;
    llvm::Value* tmgr = i->op3() ? cg()->llvmValue(i->op3()) : nullptr;

    if ( i->op2() ) {
        if ( ast::rtti::isA<type::Enum>(i->op2()) ) {
            style = cg()->llvmValue(i->op2());
        }

        else {
            auto cexpr = ast::rtti::checkedCast<expression::Constant>(i->op2());
            auto vals = ast::rtti::checkedCast<constant::Tuple>(cexpr->constant())->value();
            auto v = vals.begin();
            style = cg()->llvmValue(*v++);

            auto p = *v;

            if ( ast::rtti::isA<type::Interval>(p->type()) )
                param = cg()->llvmValue(p);
            else
                param = cg()->llvmValue(p, builder::integer::type(64));
        }
    }

    cg()->llvmProfilerStart(tag, style, param, tmgr);
}

void StatementBuilder::visit(statement::instruction::profiler::Stop* i)
{
    llvm::Value* tag = cg()->llvmValue(i->op1());

    cg()->llvmProfilerStop(tag);
}

void StatementBuilder::visit(statement::instruction::profiler::Update* i)
{
    llvm::Value* tag = cg()->llvmValue(i->op1());
    llvm::Value* arg = nullptr;

    if ( i->op2() )
        arg = cg()->llvmValue(i->op2());

    cg()->llvmProfilerUpdate(tag, arg);
}
