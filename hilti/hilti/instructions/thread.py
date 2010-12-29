# $Id$
"""

.. hlt:type:: thread

   XXX
"""

import llvm.core

import hilti.block as block
import hilti.function as function
import hilti.operand as operand

import operators
import flow

from hilti.instruction import *
from hilti.constraints import *

@hlt.instruction("thread.schedule", op1=cIntegerOfWidth(64), op2=cFunction, op3=cTuple, terminator=False)
class ThreadSchedule(Instruction):
    """
    Schedules a function call onto a virtual thread.
    """
    def _validate(self, vld):
        super(ThreadSchedule, self)._validate(vld)

        func = self.op2().value().function()
        if not flow._checkFunc(vld, self, func, [function.CallingConvention.HILTI]):
            return

        rt = func.type().resultType()

        if rt != type.Void:
            vld.error(self, "thread.schedule used to call a function that returns a value")

        flow._checkArgs(vld, self, func, self.op3())

    def _codegen(self, cg):
        op1 = self.op1().coerceTo(cg, type.Integer(64))
        vid = cg.llvmOp(op1)
        func = cg.lookupFunction(self.op2())
        args = self.op3()
        cont = cg.llvmMakeCallable(func, args)

        mgr = cg.llvmGlobalThreadMgrPtr()
        ctx = cg.llvmCurrentExecutionContextPtr()
        cg.llvmCallCInternal("__hlt_thread_mgr_schedule", [mgr, vid, cont, cg.llvmFrameExceptionAddr(), ctx])
        cg.llvmExceptionTest()

@hlt.instruction("thread.id", target=cIntegerOfWidth(64))
class ThreadID(Instruction):
    """Returns the ID of the current virtual thread. Returns -1 if executed in
    the primary thread.
    """
    def _codegen(self, cg):
        cg.llvmStoreInTarget(self, cg.llvmExecutionContextVThreadID())
