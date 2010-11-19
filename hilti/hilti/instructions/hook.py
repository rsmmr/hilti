# $Id$
"""
Hooks
~~~~~

A ``hook`` is collection of individual hook functions, all having
the same signature and grouped together under a shared name. A
caller can execute all functions making up the hook by refering to
it by that name. Example:

   hook void my_hook() {
       call Hilti::print("1st hook function");
   }

   hook void my_hook() {
       call Hilti::print("2nd hook function");
   }

   ...

   hook.run my_hook ()

   ...

This will print:

  1st hook function
  2nd hook function.

Hooks functions have a priority associated with them that determines
the order in which they are executed; those with higher priorities
will be executed first:

   hook void my_hook() &priority=-5 {
       call Hilti::print("1st hook function");
   }

   hook void my_hook() &priority=5 {
       call Hilti::print("2nd hook function");
   }

   ...

   hook.run my_hook ()

   ...

This will print:

  2nd hook function.
  1st hook function

If not explicitly defined, the default priority is zero. If multiple
hook functions have the same priority, it is undefined in which
order they are executed.

Like normal functions, hooks can receive parameters:

   hook void my_hook2(int<64> i) {
       call Hilti::print(i)
   }

   ...

   hook.run my_hook2 (42)

   ...

Also like normal functions, hooks can return values. However, unlike
normal functions, they must use the ``hook.stop`` instruction for
doing so, indicating that this will abort the hook's execution and
not run any further hook functions:

   hook int<64> my_hook3() &priority=2 {
       call Hilti::print("1st hook function")
       hook.stop 42
   }

   hook void my_hook3() &priority=1 {
       call Hilti::print("2nd hook function")
       hook.stop 21
   }

   ...

   local int<64> rc

   rc = hook.run my_hook3 ()
   call Hilti::print(rc);

   ...

This will print:

   1st hook function
   42

If a hook is declared to return a value, but no hook function
executes ``hook.stop``, the target operand will be left unchanged.

There is also a numerical, non-negative ``group` associated with
each hook function:

   hook void my_hook4() &group=1 {
       call Hilti::print("1st hook function");
   }

If not given, the default group is zero. All hook functions having
the same group can be globally disabled (and potentially later
re-enabled):

   hook.disable_group 1

   ...

   hook.enable_group 1

When a hook is run, all currently disabled hook functions are
ignored. Note that groups are indeed fully global, and
enabling/disabling is visible across all threads.

Todo: The current hook implementation works only if all hook
functions are visible to the compiler, not across independent
compilation units.
"""

import llvm.core

import hilti.util as util
import hilti.function as function

import flow
import tuple
import exception
import misc

from hilti.constraints import *
from hilti.instructions.operators import *

@type.hilti(None, 28)
class Hook(type.Function):
    """Type for hooks. Hooks are derived from ~~Function, as they share its
    general structure.

    args: list of either ~~IDs or of (~~ID, default) tuples - The hooks's
    arguments, with *default* being optional default values in the latter
    case. *default* can also be set to *None* for no default.

    resultt - ~~Type - The type of the hook's return value (~~Void for none).
    """
    def __init__(self, args, resultt, location=None):
        super(Hook, self).__init__(args, resultt, location)
        self._orig_rtype = resultt

    def origResultType(self):
        """XXX"""
        return self._orig_rtype

    def name(self):
        args = ", ".join([str(i.type()) for (i, default) in zip(self._ids, self._defaults)])
        return "hook (%s) -> %s" % (args, self._result)

class HookFunction(function.Function):
    """Type for hook functions. See ~~Function for parameters."""
    def __init__(self, ty, ident, parent, location=None):
        super(HookFunction, self).__init__(ty, ident, parent, cc=function.CallingConvention.HILTI, location=location)
        self._prio = 0
        self._group = 0

    def group(self):
        """Returns the hook group this function is part of. If not otherwise
        set, the default group is zero.

        Returns: Integer - The group.
        """
        return self._group

    def setGroup(self, group):
        """Sets the hook group this function is part of.

        group: integer - The group.
        """
        self._group = group

    def priority(self):
        """Returns the priority associated with this function. Functions with
        higher priority are executed first when a hook is run. If not
        otherwise set, the default priority is zero.

        Returns: Integer - The priority.
        """
        return self._prio

    def setPriority(self, prio):
        """Sets the priority associated with this function.

        prio: integer - The priority.
        """
        self._prio = prio

    # Overwritten from function.Function.
    def _outputAttrs(self, printer):
        if self._group:
            printer.output(" &group=%d" % self._group)

        if self._prio:
            printer.output(" &priority=%d " % self._prio)

@hlt.instruction("hook.run", op1=cHook, op2=cTuple, target=cOptional(cAny), terminator=False)
class Run(Instruction):
    """Executes the hook *op1* with arguments *op2*, storing the hook's return
    value in *target*."""
    def _resolve(self, resolver):
        super(Run, self)._resolve(resolver)

        if self.op2():
            op2 = flow._applyDefaultParams(self.op1().value(), self.op2())
            self.setOp2(op2)

    def _validate(self, vld):
        super(Run, self)._validate(vld)

        if not isinstance(self.op1().value(), id.Hook):
            vld.error(self, "argument is not a hook")
            return

        htype = self.op1().value().type()

        rt = htype.resultType()

        if self.target() and rt == type.Void():
            vld.error(self, "hook does not return a value")
            return

        if not self.target() and rt != type.Void():
            vld.error(self, "hook returns a value")
            return

        # flow._checkArgs(vld, self, htype, self.op2(), "hook")

    def _callHook(self, canonifier, func, block_current, block_next, block_exit, tmp_enabled, tmp_result, tmp_stop, tmp_tuple):

        # Check whether the group is enabled.
        op1 = operand.Constant(constant.Constant(func.group(), type.Integer(64)))
        target = tmp_enabled
        enabled = GroupEnabled(op1=op1, target=target)
        block_enabled = self._newBlock(canonifier, "is_enabled")
        block_stopped = self._newBlock(canonifier, "is_stopped")
        canonifier.addTransformedBlock(block_enabled)
        canonifier.addTransformedBlock(block_stopped)

        ifelse = flow.IfElse(op1=target,
                             op2=operand.ID(id.Local(block_enabled.name(), type.Label())),
                             op3=operand.ID(id.Local(block_next.name(), type.Label())))

        block_current.addInstruction(enabled)
        block_current.addInstruction(ifelse)

        if tmp_tuple:
            # Hook returns a result.
            block_parse_tuple = self._newBlock(canonifier, "result")
            canonifier.addTransformedBlock(block_parse_tuple)

            op1 = operand.ID(id.Function(func.name(), func.type(), func))
            op2 = self.op2()
            op3 = operand.ID(id.Local(block_parse_tuple.name(), type.Label()))
            target = tmp_tuple

            call = flow.CallTailResult(target=target, op1=op1, op2=op2, op3=op3)
            block_enabled.addInstruction(call)

            op1 = tmp_tuple
            op2 = operand.Constant(constant.Constant(0, type.Integer(64)))
            target = tmp_stop
            index0 = tuple.Index(target=target, op1=op1, op2=op2)
            block_parse_tuple.addInstruction(index0)

            op2 = operand.Constant(constant.Constant(1, type.Integer(64)))
            target = tmp_result
            index1 = tuple.Index(target=target, op1=op1, op2=op2)
            block_parse_tuple.addInstruction(index1)

            jump = flow.Jump(op1=operand.ID(id.Local(block_stopped.name(), type.Label())))
            block_parse_tuple.addInstruction(jump)

        else:
            # Hook does not return a result.
            op1 = operand.ID(id.Function(func.name(), func.type(), func))
            op2 = self.op2()
            op3 = operand.ID(id.Local(block_stopped.name(), type.Label()))
            target = tmp_stop

            call = flow.CallTailResult(target=target, op1=op1, op2=op2, op3=op3)
            block_enabled.addInstruction(call)

        ifelse = flow.IfElse(op1=tmp_stop,
                             op2=id.Local(block_exit.name(), type.Label()),
                             op3=id.Local(block_next.name(), type.Label()))

        block_stopped.addInstruction(ifelse)

    def _newBlock(self, canonifier, tag = ""):
        tag = "hook_%s" % tag if tag else "hook"
        label = canonifier.makeUniqueLabel(tag)
        b = block.Block(canonifier.currentFunction(), name=label)
        return b

    def _canonify(self, canonifier):
        super(Run, self)._canonify(canonifier)

        hook = self.op1().value()

        canonifier.deleteCurrentInstruction()

        b = self._newBlock(canonifier, "next")
        jump = flow.Jump(op1=operand.ID(id.Local(b.name(), type.Label())))
        canonifier.currentTransformedBlock().addInstruction(jump)
        canonifier.addTransformedBlock(b)

        if hook.type().origResultType() != type.Void():
            # Hook returns a result.
            rtype = type.Tuple([type.Bool(), hook.type().origResultType()])
            tmp_tuple = operand.ID(id.Local("__hook_tuple", rtype))
            tmp_result = operand.ID(id.Local("__hook_result", hook.type().origResultType()))
#            tmp_excpt = operand.ID(id.Local("__hook_excpt", type.Reference(no_hook.type())))
            canonifier.currentFunction().scope().add(tmp_tuple.value())
            canonifier.currentFunction().scope().add(tmp_result.value())
#            canonifier.currentFunction().scope().add(tmp_excpt.value())

        else:
            # Hook does not return a result.
            rtype = type.Bool()
            tmp_tuple = None
            tmp_result = None
            tmp_excpt = None

        tmp_enabled = operand.ID(id.Local("__hook_enabled", type.Bool()))
        tmp_stop = operand.ID(id.Local("__hook_stop", type.Bool()))

        exit = self._newBlock(canonifier, "exit")

        canonifier.currentFunction().scope().add(tmp_enabled.value())
        canonifier.currentFunction().scope().add(tmp_stop.value())

        for func in sorted(hook.functions(), key=lambda x: x.priority(), reverse=True):
            next = self._newBlock(canonifier)
            canonifier.addTransformedBlock(next)
            self._callHook(canonifier, func, b, next, exit, tmp_enabled, tmp_result, tmp_stop, tmp_tuple)
            b = next

        jump = flow.Jump(op1=operand.ID(id.Local(exit.name(), type.Label())))
        b.addInstruction(jump)

        canonifier.addTransformedBlock(exit)

        if hook.type().origResultType() != type.Void():
            # Hook must return a value. Check whether it has.
            block_have_result = self._newBlock(canonifier, "have_result")
            block_no_result = self._newBlock(canonifier, "no_result")
            canonifier.addTransformedBlock(block_have_result)
            canonifier.addTransformedBlock(block_no_result)

            ifelse = flow.IfElse(op1=tmp_stop,
                                 op2=operand.ID(id.Local(block_have_result.name(), type.Label())),
                                 op3=operand.ID(id.Local(block_no_result.name(), type.Label())))

            exit.addInstruction(ifelse)

            assign = Assign(target=self.target(), op1=tmp_result)
            block_have_result.addInstruction(assign)

#            excpt = New(target=tmp_excpt, op1=no_hook, op2=operand.Constant(constant.Constant(hook.name(), type.String())))
#            block_no_result.addInstruction(excpt)
#            throw = exception.Throw(op1=tmp_excpt)
#            block_no_result.addInstruction(throw)

            done = self._newBlock(canonifier, "done")
            jump1 = flow.Jump(op1=operand.ID(id.Local(done.name(), type.Label())))
            jump2 = flow.Jump(op1=operand.ID(id.Local(done.name(), type.Label())))
            block_have_result.addInstruction(jump1)
            block_no_result.addInstruction(jump2)

            canonifier.addTransformedBlock(done)

    def _codegen(self, cg):
        # This is canonified away.
        assert False

@hlt.instruction("hook.stop", op1=cOptional(cAny), terminator=True)
class Stop(Instruction):
    """Stops the execution of the current hook and returns *op1* as the hooks
    value (if one is needed)."""
    def _validate(self, vld):
        super(Stop, self)._validate(vld)

        if not isinstance(vld.currentFunction(), HookFunction):
            vld.error(self, "hook.stop can only be used inside a hook function")

        rt = vld.currentFunction().type().resultType()

        if isinstance(rt, type.Void) and self.op1():
            vld.error(self, "void hook function returns a value")
            return

        if not isinstance(rt, type.Void) and not self.op1():
            vld.error(self, "non-void hook function does not return a value")
            return

        if self.op1() and not self.op1().canCoerceTo(rt):
            vld.error(self, "return type does not match hook function definition (is %s, expected %s)" % (self.op1().type(), rt))

    def _canonify(self, canonifier):
        super(Stop, self)._canonify(canonifier)

        rt = canonifier.currentFunction().type().resultType()

        if isinstance(rt, type.Void):
            ret = flow.ReturnResult(op1=operand.Constant(constant.Constant(1, type.Bool())))
            ret.setInternal()
        else:
            elems = [operand.Constant(constant.Constant(1, type.Bool())), self.op1()]
            tuple = constant.Constant(elems, type.Tuple([e.type() for e in elems]))
            ret = flow.ReturnResult(op1=operand.Constant(tuple))
            ret.setInternal()

        canonifier.splitBlock(ret)

    def _codegen(self, cg):
        # Canonfied away.
        assert False

@hlt.instruction("hook.enable_group", op1=cIntegerOfWidth(64))
class EnableGroup(Instruction):
    """Enables the hook group given by *op1* globally."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::hook_group_enable", [self.op1(), operand.Constant(constant.Constant(1, type.Bool()))])

@hlt.instruction("hook.disable_group", op1=cIntegerOfWidth(64))
class DisableGroup(Instruction):
    """Disables the hook group given by *op1* globally."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::hook_group_enable", [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))])

@hlt.instruction("hook.group_enabled", op1=cIntegerOfWidth(64), target=cBool)
class GroupEnabled(Instruction):
    """Sets *target* to ``True`` if hook group *op1* is enabled, and to *False*
    otherwise.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::hook_group_is_enabled", [self.op1()])
        cg.llvmStoreInTarget(self, result)

