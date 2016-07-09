
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::channel::Read* i)
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
            self.blockingCodegen(cg)

    */
}

void StatementBuilder::visit(statement::instruction::channel::ReadTry* i)
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
            voidp = cg.llvmCallC("hlt::channel_read_try", [self.op1()])
            item_type = self.target().type()
            nodep = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(item_type)))
            cg.llvmStoreInTarget(self, cg.builder().load(nodep))

    */
}

void StatementBuilder::visit(statement::instruction::channel::Size* i)
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
            result = cg.llvmCallC("hlt::channel_size", [self.op1()])
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::channel::Write* i)
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
            self.blockingCodegen(cg)

    */
}

void StatementBuilder::visit(statement::instruction::channel::WriteTry* i)
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
            cg.llvmCallC("hlt::channel_write_try", [self.op1(), op2])

    */
}
