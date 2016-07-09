
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::set::Clear* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            cg.llvmCallC("hlt::set_clear", [self.op1()])

    */
}

void StatementBuilder::visit(statement::instruction::set::Exists* i)
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
            key = self.op1().type().refType().keyType()
            result = cg.llvmCallC("hlt::set_exists", [self.op1(), self.op2().coerceTo(cg, key)])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::set::Insert* i)
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
            key = self.op1().type().refType().keyType()
            cg.llvmCallC("hlt::set_insert", [self.op1(), self.op2().coerceTo(cg, key)])

    */
}

void StatementBuilder::visit(statement::instruction::set::Remove* i)
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
            key = self.op1().type().refType().keyType()
            cg.llvmCallC("hlt::set_remove", [self.op1(), self.op2().coerceTo(cg, key)])

    */
}

void StatementBuilder::visit(statement::instruction::set::Size* i)
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
            result = cg.llvmCallC("hlt::set_size", [self.op1()])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::set::Timeout* i)
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
            cg.llvmCallC("hlt::set_timeout", [self.op1(), self.op2(), self.op3()])

    */
}
