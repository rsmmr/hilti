# $Id$
"""
.. hlt:type:: timer

   A ``timer`` executes an action at a certain point of time in the future,
   where "time" is defined by a ``timer_mgr`` object with which each timer is
   associated. Each manager tracks its time as a monotonically increasing
   ``double`` value. Managers never advance their time themselves; the HILTI
   program does that explicitly.
"""

builtin_id = id

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("timer", 25, c="hlt_timer *")
class Timer(type.HeapType):
    """Type for ``timer``.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, location=None):
        super(Timer, self).__init__(location=location)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::timer_to_string"
        typeinfo.to_double = "hlt::timer_to_double"
        return typeinfo

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

@hlt.type("timer_mgr", 26, c="hlt_timer_mgr *")
class TimerMgr(type.HeapType):
    """Type for ``timer_mgr``.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, location=None):
        super(TimerMgr, self).__init__(location=location)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::timer_mgr_to_string"
        return typeinfo

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

@hlt.overload(New, op1=cType(cTimer), op2=cFunction, op3=cTuple, target=cReferenceOfOp(1))
class NewTimer(Operator):
    """Instantiates a new timer that upon expiration will run function *op2*
    with arguments *op3*. The arguments are evaluated at the time when the
    schedule instruction is executed. The called function must not return a
    value."""

    def _validate(self, vld):
        if self.op2() and not isinstance(self.op2().type().resultType(), type.Void):
            vld.error(self, "function for schedule must not return a value")

    def _codegen(self, cg):
        func = cg.lookupFunction(self.op2())
        args = self.op3()
        cont = cg.llvmMakeCallable(func, args)

        result = cg.llvmCallCInternal("__hlt_timer_new_function", [cont, cg.llvmFrameExceptionAddr(), cg.llvmCurrentExecutionContextPtr()])
        cg.llvmExceptionTest()
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("timer.update", op1=cReferenceOf(cTimer), op2=cDouble)
class Update(InstructionWithCallables):
    """Adjusts *op1*'s expiration time to *op2*.

    Raises ``TimerNotScheduled`` if the timer is not scheduled with a timer
    manager.
    """
    def _codegen(self, cg):
        cg.llvmCallC("hlt::timer_update", [self.op1(), self.op2()])

@hlt.instruction("timer.cancel", op1=cReferenceOf(cTimer))
class Cancel(Instruction):
    """Cancels the timer *op1*. This also detaches the timer from its manager,
    and allows it to be rescheduled subsequently."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::timer_cancel", [self.op1()])

@hlt.overload(New, op1=cType(cTimerMgr), target=cReferenceOfOp(1))
class NewTimerMgr(Operator):
    """Instantiates a new timer manager."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::timer_mgr_new", [])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("timer_mgr.advance", op1=cReferenceOf(cTimerMgr), op2=cDouble)
class Advance(InstructionWithCallables):
    """Advances *op1*'s notion of time to *op2*. Time can never go backwards,
    and thus the instruction has no effect if *op2* is smaller than the timer
    manager's current time."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::timer_mgr_advance", [self.op1(), self.op2()])

@hlt.instruction("timer_mgr.current", op1=cReferenceOf(cTimerMgr), target=cDouble)
class Current(Instruction):
    """Returns *op1*'s current time."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::timer_mgr_current", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("timer_mgr.expire", op1=cReferenceOf(cTimerMgr), op2=cBool)
class Expire(InstructionWithCallables):
    """Expires all timers currently associated with *op1*. If *op2* is True,
    all their actions will be executed immediately; if it's False, all the
    timers will simply be canceled."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::timer_mgr_expire", [self.op1(), self.op2()])

@hlt.instruction("timer_mgr.schedule", op1=cReferenceOf(cTimerMgr), op2=cDouble, op3=cReferenceOf(cTimer))
class Schedule(InstructionWithCallables):
    """Schedules *op2* with the timer manager *op1* to be executed at time
    *op2*. If *op2* is smaller or equal to the manager's current time, the
    timer fires immediately. Each timer can only be scheduled with a single
    time manager at a time; it needs to be canceled before it can be
    rescheduled.

    Raises ``TimerAlreadyScheduled`` if the timer is already scheduled with a
    timer manager.
    """
    def _codegen(self, cg):
        cg.llvmCallC("hlt::timer_mgr_schedule", [self.op1(), self.op2(), self.op3()])



