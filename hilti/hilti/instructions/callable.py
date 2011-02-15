# $Id$
"""
.. hlt:type:: callable

   XXX
"""

import llvm.core

import hilti.util as util
import hilti.function as function

from hilti.constraints import *
from hilti.instructions.operators import *

import bytes
import flow

@hlt.type("callable", 31, c="hlt_callable *")
class Callable(type.HeapType, type.Parameterizable):
    """Type for ``callable``.

    rtype: ~~Type - The result type yielded by calling the callable.
    """

    def __init__(self, rtype, location=None):
        super(Callable, self).__init__(location=location)
        self._rtype = rtype

    def resultType(self):
        """Returns the callable's result type."""
        return self._rtype

    ### Overridden from Type.

    def name(self):
        return "callable<%s>" % self._rtype

    def _validate(self, vld):
        super(Callable, self)._validate(vld)
        self._rtype.validate(vld)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """A ``callable`` object is mapped to ``hlt_callable *, where
        ``hlt_callable`` is itself a pointer. This double indirection allows
        us to track when a continuation has been called by setting the outer
        pointer to null.``.
        """
        return llvm.core.Type.pointer(cg.llvmTypeContinuationPtr())

    def typeInfo(self, cg):
        return cg.TypeInfo(self)

    ### Overridden from Parameterizable.

    def args(self):
        return [self._rtype]

@hlt.overload(New, op1=cType(cCallable), target=cReferenceOfOp(1))
class New(Operator):
    """Instantiates a new *callable* instance representing the function *op1*
    with arguments already bound to *op2*.
    """
    def _codegen(self, cg):
        ptr = cg.llvmMalloc(cg.llvmTypeContinuationPtr(), tag="new <callable>", location=self.location())
        null = llvm.core.Constant.null(cg.llvmTypeContinuationPtr())
        cg.builder().store(null, ptr)
        cg.llvmStoreInTarget(self, ptr)

@hlt.instruction("callable.bind", op1=cReferenceOf(cCallable), op2=cFunction, op3=cTuple)
class Bind(Instruction):
    """Binds arguments *op2* to a call of function *op1* and return the
    resulting callable."""
    def _validate(self, vld):
        super(Bind, self)._validate(vld)

        func = self.op2().value().function()

        ctype = self.op1().type().refType()
        rtype = ctype.resultType()

        if not type.canCoerceTo(func.type().resultType(), rtype):
            vld.error(self, "callable's type does match function's return type")

        if not flow._checkFunc(vld, self, func, [function.CallingConvention.HILTI]):
            return

        flow._checkArgs(vld, self, func.type(), self.op3())

    def _codegen(self, cg):
        func = self.op2().value().function()
        args = self.op3()
        cont = cg.llvmMakeCallable(func, args)

        ptr = cg.llvmOp(self.op1())
        cg.builder().store(cont, ptr)

@hlt.constraint("ref<callable<T>>")
def _call_target(ty, op, i):
    ctype = i.op1().type().refType()

    if ctype == type.Void() and op:
        return (False, "callable does not return a value")
    else:
        if not op:
            return (False, "callable returns a value")

        if not type.canCoerceTo(ctype, ty):
            return (False, "target type does not match callable's return type")

    return (True, "")

@hlt.instruction(".callable.next", target=cReferenceOf(cCallable))
class NextCallable(Instruction):
    """Internal instruction. Retrieves the next not-yet-processed
    callable registered by the C run-time.

    Note: The returned reference will remain valid only until either the next
    time this instruction or any other instruction potentially registering new
    callables is called.
    """

    def _validate(self, vld):
        super(NextCallable, self)._validate(vld)

        if self.target().type().refType().resultType() != type.Void():
            vld.error("target must be ref<callable<void>>")

    def _codegen(self, cg):
        ctx = cg.llvmCurrentExecutionContextPtr()
        c = cg.llvmCallCInternal("__hlt_callable_next", [ctx, cg.llvmFrameExceptionAddr()])
        cg.llvmStoreInTarget(self, c)
