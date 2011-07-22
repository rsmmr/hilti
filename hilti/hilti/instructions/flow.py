# $Id$
"""
"""

_doc_section = ("flow", "Flow Control")

# Todo: Where to document this:
#
# * For all |terminator| instructions, the current ~~Block is closed at their
#   location and a new ~~Block is opened.


# Todo: Should be split the overloaded call.* instruction into individual
# instructions?

import llvm.core

import hilti.block as block
import hilti.function as function
import hilti.operand as operand
import hilti.instructions.hook as hook

import operators

thread = None

from hilti.instruction import *
from hilti.constraints import *

@hlt.instruction("jump", op1=cLabel, terminator=True)
class Jump(Instruction):
    """
    Jumps unconditionally to label *op2*.
    """
    def _canonify(self, canonifier):
        super(Jump, self)._canonify(canonifier)
        canonifier.splitBlock(self)

    def _codegen(self, cg):
        cg.llvmTailCall(cg.lookupBlock(self.op1()))

@hlt.instruction("return.void", terminator=True)
class ReturnVoid(Instruction):
    """
    Returns from the current function without returning any value.
    The function's signature must not specifiy a return value.
    """
    def _validate(self, vld):
        super(ReturnVoid, self)._validate(vld)

        if self.internal():
            # Added by hook.stop.
            return

        rt = vld.currentFunction().type().resultType()
        if rt != type.Void and not isinstance(vld.currentFunction(), hook.HookFunction):
            vld.error(self, "function must return a value")

    def _canonify(self, canonifier):
        super(ReturnVoid, self)._canonify(canonifier)

        if isinstance(canonifier.currentFunction(), hook.HookFunction):
            no = constant.Constant(0, type.Bool())
            ret = ReturnResult(op1=operand.Constant(no))
            canonifier.splitBlock(ret)
            ret.setInternal()

        else:
            canonifier.splitBlock(self, add_flow_dbg=True)

    def _codegen(self, cg):
        if cg.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            succ = cg.llvmFrameNormalSucc()
            frame = cg.llvmFrameNormalFrame()
            cg.llvmTailCall(succ, frame=frame, clear=True)
        else:
            cg.builder().ret_void()

@hlt.instruction("return.result", op1=cOptional(cAny), terminator=True)
class ReturnResult(Instruction):
    """
    Returns from the current function with the given value.
    The type of the value must match the function's signature.
    """
    def _validate(self, vld):
        super(ReturnResult, self)._validate(vld)

        if self.internal():
            # Added by hook.stop.
            return

        if isinstance(vld.currentFunction(), hook.HookFunction):
            vld.error(self, "return not allowed in hook functions; use hook.stop instead")

        rt = vld.currentFunction().type().resultType()
        if isinstance(rt, type.Void):
            vld.error(self, "void function returns a value")
            return

        if not self.op1().canCoerceTo(rt):
            vld.error(self, "return type does not match function definition (is %s, expected %s)" % (self.op1().type(), rt))

    def _canonify(self, canonifier):
        super(ReturnResult, self)._canonify(canonifier)
        canonifier.splitBlock(self, add_flow_dbg=True)

    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())

        if cg.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            rtype = cg.currentFunction().type().resultType()
            succ = cg.llvmFrameNormalSucc()
            frame = cg.llvmFrameNormalFrame()

            addr = cg._llvmAddrInBasicFrame(frame.fptr(), 0, 2)
            cg.llvmTailCall(succ, frame=frame, addl_args=[(op1, rtype)], clear=True)
        else:
            cg.builder().ret(op1)

@hlt.instruction("if.else", op1=cBool, op2=cLabel, op3=cLabel, terminator=True)
class IfElse(Instruction):
    """
    Transfers control label *op2* if *op1* is true, and to *op3* otherwise.
    """
    def _canonify(self, canonifier):
        super(IfElse, self)._canonify(canonifier)
        canonifier.splitBlock(self)

    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1(), type.Bool())
        true = cg.lookupBlock(self.op2())
        false = cg.lookupBlock(self.op3())

        block_true = cg.llvmNewBlock("true")
        block_false = cg.llvmNewBlock("false")

        cg.pushBuilder(block_true)

        cg.llvmTailCall(true)
        cg.popBuilder()

        cg.pushBuilder(block_false)
        cg.llvmTailCall(false)
        cg.popBuilder()

        cg.builder().cbranch(op1, block_true, block_false)

@hlt.constraint("function")
def funcOrCallable(ty, op, i):
    (callable, err) = (cReferenceOf(cCallable))(ty, op, i)
    if callable:
        return (True, "")

    return cFunction(ty, op, i)

@hlt.constraint("(arguments)")
def tupleIfFunc(ty, op, i):
    if isinstance(i.op1().type(), type.Reference):
        return (op == None, "superflous operand 2")
    else:
        return cTuple(ty, op, i)

tupleIfFunc._optional = True

@hlt.constraint("XXX")
def tupleOrLabel(ty, op, i):
    if isinstance(i.op1().type(), type.Reference):
        return cLabel(ty, op, i)
    else:
        return cTuple(ty, op, i)

@hlt.constraint("XXX")
def labelOrNothing(ty, op, i):
    if isinstance(i.op1().type(), type.Reference):
        return cLabel(ty, op, i)
    else:
        return (op != None, "superflous argument")

labelOrNothing._optional = True

@hlt.instruction("call", op1=funcOrCallable, op2=tupleIfFunc, target=cOptional(cAny))
class Call(Instruction):
    """
    Calls *function* using the tuple in *op2* as
    arguments. The argument types as well as the target's type must
    match the function's signature.
    """

    def _resolve(self, resolver):
        super(Call, self)._resolve(resolver)

        if self.op2():
            op2 = _applyDefaultParams(self.op1().value(), self.op2())
            self.setOp2(op2)

    def _validate(self, vld):
        super(Call, self)._validate(vld)

        if isinstance(self.op1().value(), id.Function):
            func = self.op1().value().function()
            if not _checkFunc(vld, self, func, None):
                return

            _checkArgs(vld, self, func.type(), self.op2())

            rt = func.type().resultType()

        elif isinstance(self.op1().type(), type.Reference):
            rt = self.op1().type().refType().resultType()
        else:
            # Error will have been caught already.
            return

        if self.target() and rt == type.Void:
            vld.error(self, "called function does not return a value")
            return

        if not self.target() and rt != type.Void:
            vld.error(self, "called function returns a value")
            return

    def _canonify(self, canonifier):
        """
        * All ~~Call instructions are replaced with either:

         1. ~~CallTailVoid if the callee function is a HILTI function without any
            return value; or

         2. ~~CallTailResult if the callee function is a HILTI function without any
            return value; or

         3. ~~CallC if the callee function is a function with C calling convention.
         """

        super(Call, self)._canonify(canonifier)

        canonifier.deleteCurrentInstruction()

        if isinstance(self.op1().value(), id.Function):

            result_type = self.op1().type().resultType()

            func = self.op1().value().function()
            assert func and isinstance(func.type(), type.Function)

            if func.callingConvention() in (function.CallingConvention.C, function.CallingConvention.C_HILTI):
                callc = CallC(op1=self.op1(), op2=self.op2(), op3=None, target=self.target(), location=self.location())
                current_block = canonifier.currentTransformedBlock()
                current_block.addInstruction(callc)
                return

        else:
            result_type = self.op1().type().refType().resultType()

        if result_type == type.Void:
            tc = CallTailVoid(op1=self.op1(), op2=self.op2(), op3=None, location=self.location())
        else:
            tc = CallTailResult(op1=self.op1(), op2=self.op2(), op3=None, target=self.target(), location=self.location())

        new_block = canonifier.splitBlock(tc)

        i = id.Local(new_block.name(), type.Label(), location=self.location())
        label = operand.ID(i, location=self.location())
        new_block.setMayRemove(False)

        if isinstance(self.op1().value(), id.Function):
            tc.setOp3(label)
        else:
            tc.setOp2(label)

    def _codegen(self, cg):
        # Can't be called because canonicalization will transform these into
        # other calls.
        assert False

@hlt.instruction("call.c", op1=cFunction, op2=cTuple, target=cOptional(cAny))
class CallC(Instruction):
    """
    For internal use only.

    Calls the external C function using the tuple in *op2* as its arguments.
    The argument types as well as the target's type must match the function's
    signature.
    """
    def _validate(self, vld):
        super(CallC, self)._validate(vld)

        func = self.op1().value().function()
        if not _checkFunc(vld, self, func, [function.CallingConvention.C, function.CallingConvention.C_HILTI]):
            return

        rt = func.type().resultType()

        if self.target() and rt == type.Void:
            vld.error(self, "C function does not return a value")

        if not self.target() and rt != type.Void:
            vld.error(self, "C function returns a value")

        _checkArgs(vld, self, func.type(), self.op2())

    def _codegen(self, cg):
        func = cg.lookupFunction(self.op1())
        result = cg.llvmCallC(func, self.op2())
        if self.target():
            cg.llvmStoreInTarget(self, result)

@hlt.instruction("call.tail.void", op1=funcOrCallable, op2=tupleOrLabel, op3=labelOrNothing, terminator=True)
class CallTailVoid(Instruction):
    """
    For internal use only.

    Calls *function* using the tuple in *op2* as arguments. The
    argument types must match the function's signature, and the
    function must not return a value. Different than *Call()*,
    *CallTailVoid* must be the *last instruction* of a *Block*.
    After the called function returns, control is passed to
    block *op3*.

    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions.
    """

    def _validate(self, vld):
        super(CallTailVoid, self)._validate(vld)

        if isinstance(self.op1().value(), id.Function):
            func = self.op1().value().function()
            if not _checkFunc(vld, self, func, [function.CallingConvention.HILTI]):
                return

            _checkArgs(vld, self, func.type(), self.op2())

            rt = func.type().resultType()
        else:
            rt = self.op1().type().refType().resultType()

        if rt != type.Void:
            vld.error(self, "call.tail.void calls a function returning a value")

    def _codegen(self, cg):
        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            args = self.op2()

            succ = cg.lookupBlock(self.op3())
            llvm_succ = cg.llvmFunctionForBlock(succ)

            frame = cg.llvmMakeFunctionFrame(func, args, llvm_succ)
            cg.llvmTailCall(func, frame=frame)

        else:
            succ = cg.lookupBlock(self.op2())
            llvm_succ = cg.llvmFunctionForBlock(succ)

            ptr = cg.llvmOp(self.op1())
            cg.llvmCallCallable(cg.builder().load(ptr), llvm_succ)

@hlt.instruction("call.tail.result", op1=funcOrCallable, op2=tupleOrLabel, op3=labelOrNothing, target=cOptional(cAny), terminator=True)
class CallTailResult(Instruction):
    """
    For internal use only.

    Like *Call()*, calls *function* using the tuple in *op2* as
    arguments. The argument types must match the function's
    signature. The function must return a result value and it's type
     must likewise match the function's signature. Different than
    *Call()*, *CallTailVoid* must be the *last instruction* of a
    *Block*.  After the called function returns, control is passed
    to block *op3*.

    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions.
    """
    def _validate(self, vld):
        super(CallTailResult, self)._validate(vld)

        if isinstance(self.op1().value(), id.Function):
            func = self.op1().value().function()
            if not _checkFunc(vld, self, func, [function.CallingConvention.HILTI]):
                return

            _checkArgs(vld, self, func.type(), self.op2())

            rt = func.type().resultType()
        else:
            rt = self.op1().type().refType().resultType()

        if rt == type.Void:
            vld.error(self, "call.tail.result calls a function that does not return a value")

    def _codegen(self, cg):
        args = self.op2()

        # Build a helper function that receives the result.
        result_name = cg.nameFunctionForBlock(cg.currentBlock()) + "_result"

        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            result_type = cg.llvmType(func.type().resultType())

            succ = cg.lookupBlock(self.op3())
            llvm_succ = cg.llvmFunctionForBlock(succ)
        else:
            result_type = cg.llvmType(self.op1().type().refType().resultType())

            succ = cg.lookupBlock(self.op2())
            llvm_succ = cg.llvmFunctionForBlock(succ)

        result_func = cg.llvmNewHILTIFunction(cg.currentFunction(), result_name, [("__result", result_type)])
        result_block = result_func.append_basic_block("")

        cg.pushBuilder(result_block)
        cg.llvmStoreLocalVar(self.target().value(), result_func.args[3], frame=result_func.args[0])
        fdesc = cg.makeFrameDescriptor(result_func.args[0], result_func.args[1])

        cg.llvmTailCall(llvm_succ, frame=fdesc, ctx=result_func.args[2]) # XXX
        cg.builder().ret_void()
        cg.popBuilder()

        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            frame = cg.llvmMakeFunctionFrame(func, args, result_func)
            cg.llvmTailCall(func, frame=frame)
        else:
            ptr = cg.llvmOp(self.op1())
            cg.llvmCallCallable(cg.builder().load(ptr), result_func)

@hlt.instruction("yield", terminator=True)
class Yield(Instruction):
    """Yields processing back to the current scheduler, to be resumed later.
    If running in a virtual thread other than zero, this instruction yield to
    other virtual threads running within the same physical thread. Of running
    in virtual thread zero (or in non-threading mode), returns execution back
    to the calling C function (see interfacing with C).
    """
    def _canonify(self, canonifier):
        super(Yield, self)._canonify(canonifier)
        canonifier.splitBlock(self, add_flow_dbg=True)

    def _codegen(self, cg):
        next = cg.currentBlock().next()
        next = cg.currentFunction().lookupBlock(next)
        assert next
        succ = cg.llvmFunctionForBlock(next)
        cg.llvmYield(succ)

@hlt.instruction("yield.until", terminator=True, op1=cReferenceOf(cBlockable))
class YieldUntil(Instruction):
    """XXX"""
    def _canonify(self, canonifier):
        super(YieldUntil, self)._canonify(canonifier)
        canonifier.splitBlock(self, add_flow_dbg=True)

    def _codegen(self, cg):
        next = cg.currentBlock().next()
        next = cg.currentFunction().lookupBlock(next)
        assert next
        succ = cg.llvmFunctionForBlock(next)

        op1 = cg.llvmOp(self.op1())
        blockable = cg.llvmBlockable(self.op1().type(), op1)
        cg.llvmYield(succ, blockable=blockable)

@hlt.constraint("( (value, destination), ...)")
def _switchTuple(ty, op, i):

    if not isinstance(ty, type.Tuple):
        return (False, "operand 3 must be a tuple")

    ops = op.value().value()

    for j in range(0, len(ops)):
        op = ops[j]

        if not isinstance(op.type(), type.Tuple) or len(op.type().types()) != 2:
            return (False, "tuple element %d is not a 2-tuple" % (j+1))

        opp = op.value().value()

        if not opp[0].canCoerceTo(i.op1().type()):
            return (False, "all values must be of same type as operand 1 (value %d is not)" % (j+1))

        if not isinstance(opp[1].type(), type.Label):
            return (False, "all destination must be labels (destination %d is not)" % (j+1))

    return (True, "")

@hlt.instruction("switch", op1=cValueType, op2=cLabel, op3=_switchTuple, terminator=True)
class Switch(Instruction):
    """Branches to one of several alternatives. *op1* determines which
    alternative to take.  *op3* is a tuple of giving all alternatives as
    2-tuples *(value, destination)*. *value* must be of the same type as
    *op1*, and *destination* is a block label. If *value* equals *op1*,
    control is transfered to the corresponding block. If multiple alternatives
    match *op1*, one of them is taken but it's undefined which one. If no
    alternative matches, control is transfered to block *op2*.
    """
    def _canonify(self, canonifier):
        super(Switch, self)._canonify(canonifier)
        canonifier.splitBlock(self)

    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        optype = self.op1().type()
        default = cg.currentFunction().lookupBlock(self.op2().value().name())
        alts = self.op3().value().value()
        vals = [op.value().value()[0] for op in alts]
        names = [op.value().value()[1].value().name() for op in alts]

        dests = [cg.currentFunction().lookupBlock(name) for name in names]
        blocks = [cg.llvmNewBlock("switch-%s" % name) for name in names]

        for (d, b) in zip(dests, blocks):
            cg.pushBuilder(b)
            cg.llvmTailCall(d)
            cg.popBuilder()

        default_block = cg.llvmNewBlock("switch-default")
        cg.pushBuilder(default_block)
        cg.llvmTailCall(default)
        cg.popBuilder()

        # We optimize for integers here, for which LLVM directly provides a switch
        # statement.
        if isinstance(optype, type.Integer):
            switch = cg.builder().switch(op1, default_block)
            for (val, block) in zip(vals, blocks):
                switch.add_case(cg.llvmOp(val, coerce_to=optype), block)

        else:
            # In all other cases, we build an if-else chain using the type's
            # standard comparision operator.
            tmp = cg.builder().alloca(llvm.core.Type.int(1))
            target = operand.LLVM(tmp, type.Bool())

            for (val, block) in zip(vals, blocks):
                # We build an artifical equal operator; just the types need to match.
                i = operators.Equal(target=target, op1=self.op1(), op2=val)
                i.codegen(cg)
                match = cg.builder().icmp(llvm.core.IPRED_EQ, cg.llvmConstInt(1, 1), cg.builder().load(tmp))

                next_block = cg.llvmNewBlock("switch-chain")
                cg.builder().cbranch(match, block, next_block)
                cg.pushBuilder(next_block)

            cg.builder().branch(default_block)

def _getStandardFrame(cg):
    # Get the reusable global portions of the standard frame.
    cont_normal = cg.llvmCFunctionInternal("__hlt_standard_cont_normal")
    cont_normal_frame = cg.llvmCFunctionInternal("__hlt_standard_frame")
    cont_except = cg.llvmCFunctionInternal("__hlt_standard_cont_except")

    # Construct the frame for the exception continuation. (This can't be reused.)
    cont_except_frame = cg.llvmMalloc(cg.llvmTypeBasicFrame(), tag="<exception frame>")
    cg.llvmFrameClearException(cont_except_frame)

    return (cont_normal, cont_normal_frame, cont_except, cont_except_frame)

def _applyDefaultParams(func, tuple):
    """Adds missing arguments to a call's parameter tuple if
    default values are known.

    func: ~~Function - The function which is called.
    tuple: ~~Constant - The call's argument operand.

    Returns: ~~Constant - The new arguments tuple, with all defaults
    filled in.
    """

    if not isinstance(func.type(), type.Function):
        # Something's wrong. We'll catch it later.
        return tuple

    if not isinstance(tuple.type(), type.Tuple):
        # Something's wrong. We'll catch it later.
        return tuple

    args = tuple.value().value()
    defaults = [t[1] for t in func.type().argsWithDefaults()]

    if len(args) >= len(defaults):
        return tuple

    new_args = args

    for i in range(len(args), len(defaults)):
        d = defaults[i]
        if d:
            new_args += [d]

    const = constant.Constant(new_args, type.Tuple([t.type() for t in new_args]))
    return operand.Constant(const, location=tuple.location())

def _checkFunc(vld, i, func, cc, schedule=False):
    if not isinstance(func, function.Function):
        vld.error(i, "not a function name")
        return False

    if cc and not func.callingConvention() in cc:
        vld.error(i, "call to function with incompatible calling convention")

    if vld.currentFunction().callingConvention() != function.CallingConvention.HILTI and \
       func.callingConvention() == function.CallingConvention.HILTI:
           vld.error(i, "cannot call HILTI function from non-HILTI function")

    if not schedule and func.threadScope():
        global thread
        if not thread:
            import thread as thread_mod
            thread = thread_mod

        if not thread._checkCall(vld.currentFunction(), func):
           vld.error(i, "cannot call function with a different scope; use thread.schedule")

           return True

def _checkArgs(vld, i, ftype, op, txt="function"):
    try:
        if not isinstance(op.type(), type.Type):
            vld.error(i, "%s arguments must be a tuple" % txt)
            return

        args = op.value().value()
        ids = ftype.args()

        if len(args) != len(ids):
            vld.error(i, "wrong number of arguments for %s" % txt)
            return

        #print "proto: ", [i.type() for i in ids]
        #print "call : ", [i.type() for i in args]

        for j in range(len(args)):
            if not args[j].canCoerceTo(ids[j].type()):
                vld.error(i, "wrong type for %s argument %d in call (is %s, expected %s)" % (txt, j+1, args[j].type(), ids[j].type()))

    except:
        # Reported somewhere else.
        pass
