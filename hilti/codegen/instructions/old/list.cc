
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::list::Back* i)
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
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_back", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

*/
}

void StatementBuilder::visit(statement::instruction::list::Erase* i)
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
        cg.llvmCallC("hlt::list_erase", [self.op1()])

*/
}

void StatementBuilder::visit(statement::instruction::list::Front* i)
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
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_front", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

*/
}

void StatementBuilder::visit(statement::instruction::list::Insert* i)
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
        op1 = self.op1().coerceTo(cg, self.op2().type().derefType())
        cg.llvmCallC("hlt::list_insert", [op1, self.op2()])

*/
}

void StatementBuilder::visit(statement::instruction::list::PopBack* i)
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
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_pop_back", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

*/
}

void StatementBuilder::visit(statement::instruction::list::PopFront* i)
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
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_pop_front", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

*/
}

void StatementBuilder::visit(statement::instruction::list::PushBack* i)
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
        cg.llvmCallC("hlt::list_push_back", [self.op1(), op2])

*/
}

void StatementBuilder::visit(statement::instruction::list::PushFront* i)
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
        cg.llvmCallC("hlt::list_push_front", [self.op1(), op2])

*/
}

void StatementBuilder::visit(statement::instruction::list::Size* i)
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
        result = cg.llvmCallC("hlt::list_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::list::Timeout* i)
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
        cg.llvmCallC("hlt::list_timeout", [self.op1(), self.op2(), self.op3()])

*/
}

