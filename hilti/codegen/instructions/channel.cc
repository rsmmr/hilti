
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::channel::New* i)
{
   auto etype = ast::as<type::Channel>(typedType(i->op1()))->argType();
   auto op1 = builder::type::create(etype);

   auto op2 = i->op2();

   if ( ! op2 )
       op2 = builder::integer::create(0);

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::channel_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::channel::Read* i)
{
    // TODO: Blocking instruction not implemented.
    abort();
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
 def cCall(self, cg):
   return ("hlt::channel_read_try", [self.op1()])

    def cResult(self, cg, result):
        item_type = self.target().type()
        nodep = cg.builder().bitcast(result, llvm.core.Type.pointer(cg.llvmType(item_type)))
        cg.llvmStoreInTarget(self, cg.builder().load(nodep))
        *
*/

/*
    def _codegen(self, cg):
        self.blockingCodegen(cg)

*/
}

void StatementBuilder::visit(statement::instruction::channel::ReadTry* i)
{
    auto etype = ast::as<type::Channel>(referencedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto r = cg()->llvmCall("hlt::channel_read_try", args);
    auto result = builder()->CreateBitCast(r, cg()->llvmTypePtr(cg()->llvmType(etype)));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::channel::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::channel_size", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::channel::Write* i)
{
   // TODO: Blocking instruction not implemented.
   abort();

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

     def cCall(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        return ("hlt::channel_write_try", [self.op1(), op2])


*/
/*
    def _codegen(self, cg):
        self.blockingCodegen(cg)

*/
}

void StatementBuilder::visit(statement::instruction::channel::WriteTry* i)
{
    auto etype = ast::as<type::Channel>(referencedType(i->op1()))->argType();

    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::channel_write_try", args);
}

