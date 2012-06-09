
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::ioSource::New* i)
{
   // TODO: Not implemented yet.
   abort();

}

void StatementBuilder::visit(statement::instruction::ioSource::Close* i)
{
   // TODO: Not implemented yet.
   abort();

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
        _makeSwitch(cg, self.op1().type().refType(), "close", [self.op1()], False)

*/
}

void StatementBuilder::visit(statement::instruction::ioSource::Read* i)
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
        func = _funcName("read_try", self.op1().type().refType().kind())
        args = [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        result = _checkExhausted(cg, self, self.op1(), result, make_iters=True)
        cg.llvmStoreInTarget(self, result)
        *

*/


/*
    def _codegen(self, cg):
        self.blockingCodegen(cg)

*/
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Begin* i)
{
/*
@hlt.overload(Begin, op1=cReferenceOf(cioSource), target=cIteratorioSource)
class Begin(BlockingOperator):
    """Returns an iterator for iterating over all elements of a packet source
    *op1*. The instruction will block until the first element becomes
    available."""
    def _canonify(self, canonifier):
        Instruction._canonify(self, canonifier)
        self.blockingCanonify(canonifier)

    def _codegen(self, cg):
        self.blockingCodegen(cg)

    def cCall(self, cg):
        func = _funcName("read_try", self.op1().type().refType().kind())
        args = [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        result = _checkExhausted(cg, self, self.op1(), result, make_iters=True)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::iterIOSource::End* i)
{
/*
@hlt.overload(End, op1=cReferenceOf(cioSource), target=cIteratorioSource)
class End(Operator):
    """Returns an iterator representing an exhausted packet source *op1*."""
    def _codegen(self, cg):
        ty = self.op1().type().refType()
        result = _makeIterator(cg, ty, None, None)
        cg.llvmStoreInTarget(self, result)
*/
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Incr* i)
{
/*
@hlt.overload(Incr, op1=cIteratorioSource, target=cIteratorioSource)
class Incr(BlockingOperator):
    """Advances the iterator to the next element, or the end position if
    already exhausted. The instruction will block until the next element
    becomes available.
    """
    def _canonify(self, canonifier):
        Instruction._canonify(self, canonifier)
        self.blockingCanonify(canonifier)

    def _codegen(self, cg):
        self.blockingCodegen(cg)

    def cCall(self, cg):
        ty = self.op1().type().parentType()
        src = operand.LLVM(cg.llvmExtractValue(cg.llvmOp(self.op1()), 0), type.Reference(ty))
        func = _funcName("read_try", ty.kind())
        args = [src, operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        ty = self.op1().type().parentType()
        src = operand.LLVM(cg.llvmExtractValue(cg.llvmOp(self.op1()), 0), type.Reference(ty))
        result = _checkExhausted(cg, self, src, result, make_iters=True)
        cg.llvmStoreInTarget(self, result)
*/
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Equal* i)
{
/*
@hlt.overload(Equal, op1=cIteratorioSource, op2=cIteratorioSource, target=cBool)
class Equal(Operator):
    """Returns true if *op1* and *op2* refer to the same element.
    """
    def _codegen(self, cg):
        elem1 = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        payload1 = cg.llvmExtractValue(elem1, 1)
        elem2 = cg.llvmExtractValue(cg.llvmOp(self.op2()), 1)
        payload2 = cg.llvmExtractValue(elem2, 1)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, payload1, payload2)
        cg.llvmStoreInTarget(self, result)
*/
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Deref* i)
{
/*
@hlt.overload(Deref, op1=cIteratorioSource, target=cDerefTypeOfOp(1))
class Deref(Operator):
    """Returns the element the iterator is pointing at as a tuple ``(double,
    ref<bytes>)``.
    """
    def _codegen(self, cg):
        elem = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        cg.llvmStoreInTarget(self, elem)
*/
}
