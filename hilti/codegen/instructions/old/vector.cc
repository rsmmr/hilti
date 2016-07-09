
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::vector::Get* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);

        auto result = builder()->

        cg()->llvmStore(i, result);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());

        auto result = cg()->llvmCall("hlt::X", args);

        cg()->llvmStore(i, result);
    */


    /*
        def _codegen(self, cg):
            t = self.op1().type().refType().itemType()
            op2 = self.op2().coerceTo(cg, type.Integer(64))
            voidp = cg.llvmCallC("hlt::vector_get", [self.op1(), op2])
            casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
            cg.llvmStoreInTarget(self, cg.builder().load(casted))

    */
}

void StatementBuilder::visit(statement::instruction::vector::Set* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
            cg.llvmCallC("hlt::vector_push_back", [self.op1(), op2])

    */
}

void StatementBuilder::visit(statement::instruction::vector::Reserve* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            op2 = self.op2().coerceTo(cg, type.Integer(64))
            cg.llvmCallC("hlt::vector_reserve", [self.op1(), op2])

    */
}

void StatementBuilder::visit(statement::instruction::vector::Set* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
        auto op3 = cg()->llvmValue(i->op3(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        args.push_back(i->op3());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            op2 = self.op2().coerceTo(cg, type.Integer(64))
            op3 = self.op3().coerceTo(cg, self.op1().type().refType().itemType())
            cg.llvmCallC("hlt::vector_set", [self.op1(), op2, op3])

    */
}

void StatementBuilder::visit(statement::instruction::vector::Size* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);

        auto result = builder()->

        cg()->llvmStore(i, result);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());

        auto result = cg()->llvmCall("hlt::X", args);

        cg()->llvmStore(i, result);
    */


    /*
        def _codegen(self, cg):
            result = cg.llvmCallC("hlt::vector_size", [self.op1()])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::vector::Timeout* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
        auto op3 = cg()->llvmValue(i->op3(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        args.push_back(i->op3());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            cg.llvmCallC("hlt::vector_timeout", [self.op1(), self.op2(), self.op3()])

    */
}
