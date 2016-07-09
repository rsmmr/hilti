
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::map::Clear* i)
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
            cg.llvmCallC("hlt::map_clear", [self.op1()])

    */
}

void StatementBuilder::visit(statement::instruction::map::Default* i)
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
            value = self.op1().type().refType().valueType()
            cg.llvmCallC("hlt::map_default", [self.op1(), self.op2().coerceTo(cg, value)])

    */
}

void StatementBuilder::visit(statement::instruction::map::Exists* i)
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
            result = cg.llvmCallC("hlt::map_exists", [self.op1(), self.op2().coerceTo(cg, key)])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::map::Get* i)
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
            value = self.op1().type().refType().valueType()
            voidp = cg.llvmCallC("hlt::map_get", [self.op1(), self.op2().coerceTo(cg, key)])
            casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(value)))
            cg.llvmStoreInTarget(self, cg.builder().load(casted))

    */
}

void StatementBuilder::visit(statement::instruction::map::GetDefault* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
        auto op3 = cg()->llvmValue(i->op3(), X);

        auto result = builder()->

        cg()->llvmStore(i, result);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        args.push_back(i->op3());

        auto result = cg()->llvmCall("hlt::X", args);

        cg()->llvmStore(i, result);
    */


    /*
        def _codegen(self, cg):
            key = self.op1().type().refType().keyType()
            value = self.op1().type().refType().valueType()
            voidp = cg.llvmCallC("hlt::map_get_default", [self.op1(), self.op2().coerceTo(cg, key),
       self.op3().coerceTo(cg, value)])
            casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(value)))
            cg.llvmStoreInTarget(self, cg.builder().load(casted))

    */
}

void StatementBuilder::visit(statement::instruction::map::Insert* i)
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
            key = self.op1().type().refType().keyType()
            value = self.op1().type().refType().valueType()
            cg.llvmCallC("hlt::map_insert", [self.op1(), self.op2().coerceTo(cg, key),
       self.op3().coerceTo(cg, value)])

    */
}

void StatementBuilder::visit(statement::instruction::map::Remove* i)
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
            cg.llvmCallC("hlt::map_remove", [self.op1(), self.op2().coerceTo(cg, key)])

    */
}

void StatementBuilder::visit(statement::instruction::map::Size* i)
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
            result = cg.llvmCallC("hlt::map_size", [self.op1()])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::map::Timeout* i)
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
            cg.llvmCallC("hlt::map_timeout", [self.op1(), self.op2(), self.op3()])

    */
}
