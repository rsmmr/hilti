# $Id$
"""
XXX
"""

_doc_section = ("debug", "Debugging")

import llvm.core

import hilti.util as util
import hilti.constant as constant

import hilti.instructions.string as string

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.instruction("debug.msg", op1=cString, op2=cString, op3=cTuple)
class Msg(Instruction):
    """If compiled in debug mode, prints a debug message to stderr. The
    message is only printed of the debug stream *op1* has been activated.
    *op2* is printf-style format string and *op3* the correponding arguments.
    """
    def _codegen(self, cg):
        if cg.debugLevel() == 0:
            return

        cg.llvmCallC("hlt::debug_printf", [self.op1(), self.op2(), self.op3()])

@hlt.instruction("debug.assert", op1=cBool, op2=cOptional(cString))
class Assert(Instruction):
    """If compiled in debug mode, verifies that *op1* is true. If not, throws
    an ~~AssertionError exception with *op2* as its value if given.
    """
    def _codegen(self, cg):
        if cg.debugLevel() == 0:
            return

        op1 = cg.llvmOp(self.op1())
        block_true = cg.llvmNewBlock("true")
        block_false = cg.llvmNewBlock("false")

        cg.builder().cbranch(op1, block_true, block_false)

        cg.pushBuilder(block_false)
        arg = cg.llvmOp(self.op2()) if self.op2() else string._makeLLVMString(cg, "")
        cg.llvmRaiseExceptionByName("hlt_exception_assertion_error", self.location(), arg)
        cg.popBuilder()

        cg.pushBuilder(block_true)
        # Leave on stack.

@hlt.instruction("debug.internal_error", op1=cString, terminator=True)
class InternalError(Instruction):
    """Throws an InternalError exception, setting *op1* as the its argument.

    Note: This is just a convenience instruction; one can also raise the
    exception directly.
    """
    def _canonify(self, canonifier):
        canonifier.splitBlock(self)

    def _codegen(self, cg):
        arg = cg.llvmOp(self.op1())
        cg.llvmRaiseExceptionByName("hlt_exception_internal_error", self.location(), arg)

@hlt.instruction("debug.push_indent")
class InternalError(Instruction):
    """Increases the indentation in debugging output by one level.

    Note: The indentation level is global across all debug streams, but
    separately tracked for each thread.
    """
    def _codegen(self, cg):
        cg.llvmDebugPushIndent()

@hlt.instruction("debug.pop_indent")
class InternalError(Instruction):
    """Decreases the indentation in debugging output by one level.

    Note: The indentation level is global across all debug streams, but
    separately tracked for each thread.
    """
    def _codegen(self, cg):
        cg.llvmDebugPopIndent()

@hlt.instruction("debug.print_frame")
class PrintFrame(Instruction):
    """An internal debugging instruction that prints out pointers making up
    the current stack frame.
    """
    def _codegen(self, cg):
        cg.llvmDebugPrintPtr("dbg.print-frame: normal succ ", cg.llvmFrameNormalSucc())
        cg.llvmDebugPrintPtr("dbg.print-frame: normal frame", cg.llvmFrameNormalFrame().fptr())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt succ  ", cg.llvmFrameExcptSucc())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt frame ", cg.llvmFrameExcptFrame().fptr())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt ptr",    cg.llvmFrameException())

def message(stream, fmt, args = []):
    """Helpers function to create a ~~Msg instruction.

    stream: string - The name of the debug stream.
    msg: string - The format string.
    args: optional list of ~~Operand - The list of format arguments.
    """

    return Msg(op1=operand.Constant(constant.Constant(stream, type.String())),
               op2=operand.Constant(constant.Constant(fmt, type.String())),
               op3=operand.Constant(constant.Constant(args, type.Tuple([op.type() for op in args]))))


