
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::caddr::Function* i)
{
    auto op1 = ast::as<expression::Function>(i->op1());
    assert(op1);

    auto func = op1->function();
    assert(func);

    auto ftype = func->type();

    llvm::Value* v1 = 0;
    llvm::Value* v2 = 0;

    switch ( ftype->callingConvention() ) {
     case type::function::HILTI: {
         auto pair = cg()->llvmBuildCWrapper(func);
         v1 = cg()->builder()->CreateBitCast(pair.first, cg()->llvmTypePtr());
         v2 = cg()->builder()->CreateBitCast(pair.second, cg()->llvmTypePtr());
         break;
     }

     case type::function::HILTI_C: {
         v1 = cg()->llvmValue(op1);
         v2 = cg()->llvmConstNull(cg()->llvmTypePtr());
         break;
     }

     default:
        internalError("caddr.function does not yet support non HILTI/HILTI-C functions");
    }

    CodeGen::value_list vals = { v1, v2 };
    auto result = cg()->llvmValueStruct(vals);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::caddr::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

