# $Id$
"""
XXX
"""

_doc_section = ("profiler", "Profiling")

import llvm.core

import hilti.util as util
import hilti.constant as constant

import hilti.instructions.string as string

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.constraint("XXX")
def _int64_or_interval(ty, op, i):
    if isinstance(ty, type.Interval):
        return (True, "")
    else:
        return cIntegerOfWidth(64)(ty, op, i)

@hlt.constraint("Hilti::ProfileStyle | (Hilti::ProfileStyle, int<64>)")
def _style(ty, op, i):
    # FIXME: We need to check here for the right combinations of style and
    # argument (and use of the right enum).

    if isinstance(ty, type.Enum):
        return (True, "")

    if isinstance(ty, type.Tuple):
        return  cIsTuple([cEnum, _int64_or_interval])(ty, op, i)

    return (False, "is not a valid profiler style")

@hlt.instruction("profiler.start", op1=cString, op2=cOptional(_style), op3=cOptional(cReferenceOf(cTimerMgr)))
class Start(Instruction):
    """Starts collecting profiling with a profilter associated with tag *op1*,
    *op2* specifies a ``Hilti:: ProfileStyle``, defining when automatic
    snapshots should be taken of the profiler's counters; the first element of
    *op2* is the style, and the second its argument if one is needed (if not,
    the argument is simply ignored). If *op3* is given, it attaches a
    ``timer_mgr`` to the started profiling, which will then (1) record times
    according to the timer manager progress, and (2) can trigger snaptshots
    according to its time.

    If a tag is given for which a profiler has already been started, this
    instruction increases the profiler's level by one, and ``profiler.stop``
    command will only record information if it has been called a matching
    number of times. 

    If profiling support is not compiled in, this instruction is a no-op.
    """
    def _codegen(self, cg):
        if cg.profileLevel() == 0:
            return

        tag = (cg.llvmOp(self.op1()), self.op1().type())

        if self.op2():
            if isinstance(self.op2().type(), type.Enum):
                style = self.op2()
                param = (cg.llvmConstInt(0, 64), type.Integer(64))

            else:
                style = self.op2().value().value()[0]
                param = self.op2().value().value()[1]

                if isinstance(param.type(), type.Interval):
                    param = param.type().llvmConstant(cg, param.value()), type.Integer(64)

        else:
            style = operand.ID(cg.currentModule().scope().lookup("Hilti::ProfileStyle::Standard"))
            param = (cg.llvmConstInt(0, 64), type.Integer(64))

        if self.op3():
            tmgr = self.op3()
        else:
            tmgr = (llvm.core.Constant.null(cg.llvmTypeGenericPointer()), type.Reference(type.TimerMgr()))

        cg.llvmCallC("hlt::profiler_start", [tag, style, param, tmgr])

@hlt.instruction("profiler.stop", op1=cString, terminator=True)
class Stops(Instruction):
    """Stops the profiler associated with the tag *op1*, recording its current
    state to disk. However, a profiler is only stopped if ``stop`` has been
    called as often as it has previously been started with ``profile.start``.

    If profiling support is not compiled in, this instruction is a no-op.
    """
    def _codegen(self, cg):
        if cg.profileLevel() == 0:
            return

        cg.llvmCallC("hlt::profiler_stop", [self.op1()])

@hlt.instruction("profiler.update", op1=cString, op2=cOptional(cIntegerOfWidth(64)))
class Stops(Instruction):
    """Records a snapshot of the current state of the profiler associated with
    tag *op1* (in ``STANDARD`` mode, otherwise according to the style).  If
    *op2* is given, the profiler's user-defined counter is adjusted by that
    amount.

    If profiling support is not compiled in, this instruction is a no-op.
    """
    def _codegen(self, cg):
        if cg.profileLevel() == 0:
            return

        if self.op2():
            op2 = (cg.llvmOp(self.op2()), self.op2().type())
        else:
            op2 = (cg.llvmConstInt(0, 64), type.Integer(64))

        cg.llvmCallC("hlt::profiler_update", [self.op1(), op2])

