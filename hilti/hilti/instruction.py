# $Id$

builtin_type = type
builtin_id = id

import constant
import node
import operand
import type
import block
import util

import instructions.flow

import llvm.core

class Instruction(node.Node):
    """Base class for all instructions supported by the HILTI language.

    To create a new instruction do however *not* derive directly from
    Instruction but use the ~~instruction decorator.

    op1: ~~Operand - The instruction's first operand, or None if unused.
    op2: ~~Operand - The instruction's second operand, or None if unused.
    op3: ~~Operand - The instruction's third operand, or None if unused.
    target: ~~ID - The instruction's target, or None if unused.
    location: ~~Location - A location to be associated with the instruction.
    """

    _signature = None

    def __init__(self, op1=None, op2=None, op3=None, target=None, location=None):
        assert not target or isinstance(target, operand.ID) or isinstance(target, operand.LLVM)

        super(Instruction, self).__init__(location)
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._location = location
        self._internal = False
        self._disable_target = False
        self._llvm_target = None

        cb = self.signature().callback() if self.signature() else None
        if cb:
            cb(self)

    def signature(self):
        """Returns the instruction's signature.

        Returns: ~~Signature - The signature.
        """
        return self.__class__._signature

    def name(self):
        """Returns the instruction's name. The name is the mnemonic as used in
        a HILTI program.

        Returns: string - The name.
        """
        return self.signature().name()

    def op1(self):
        """Returns the instruction's first operand.

        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op1

    def op2(self):
        """Returns the instruction's second operand.

        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op2

    def op3(self):
        """Returns the instruction's third operand.

        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op3

    def target(self):
        """Returns the instruction's target.

        Returns: ~~ID - The target, or None if unused.
        """
        return self._target

    def setOp1(self, op):
        """Sets the instruction's first operand.

        op: ~~Operand - The new operand to set.
        """
        self._op1 = op

    def setOp2(self, op):
        """Sets the instruction's second operand.

        op: ~~Operand - The new operand to set.
        """
        self._op2 = op

    def setOp3(self, op):
        """Sets the instruction's third operand.

        op: ~~Operand - The new operand to set.
        """
        self._op3 = op

    def setTarget(self, target):
        """Sets the instruction's target.

        op: ~~ID - The new operand to set.
        """
        assert not target or isinstance(target, operand.ID)
        self._target = target

    def setInternal(self):
        """Marks the instruction as internally generated."""
        self._internal = True

    def internal(self):
        """Returns whether the instruction has been marked as internally
        generated."""
        return self._internal

    def disableTarget(self):
        """Instructs the code generator to not store the result in the target
        operand. This is useful if the operator is used internally (rather
        than parsed from user code), and the result is to be retrieved via
        ~~llvmTarget.
        """
        self._disable_target = True

    def targetDisabled(self):
        """Returns whether storing in the target operand has been disabled.
        See ~~disableTarget.

        Returns: bool - True if it has been disabled.
        """
        return self._disable_target

    def llvmTarget(self):
        """Returns the result of the instruction as stored in the target
        operand. If ~~disableTarget has been called, the result will actually
        not haven been stored there but can still be retrieved. This method
        must only be called after code generation for the operator and the
        returns value is only valid withon the last generated LLVM block.

        Returns: llvm.core.Value - The target value.
        """
        print repr(self)
        assert self._llvm_target
        return self._llvm_target

    def _storeLLVMResult(self, result):
        """Method for the code generator to store the target value returned by
        ~~llvmTarget."""
        assert result
        self._llvm_target = result

    def __str__(self):
        target = "(%s) = " % self._target if self._target else ""
        op1 = " (%s)" % self._op1 if self._op1 else ""
        op2 = " (%s)" % self._op2 if self._op2 else ""
        op3 = " (%s)" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self.signature().name(), op1, op2, op3)

    ### Overridden from Node.

    def _resolve(self, resolver):
        for op in (self._op1, self._op2, self._op3, self._target):
            if op:
                op.resolve(resolver)

    def _validate(self, vld):
        errors = vld.errors()

        for op in (self._op1, self._op2, self._op3, self._target):
            if op:
                op.validate(vld)

        (success, errormsg) = self._signature.matchWithInstruction(self)
        if not success:
            vld.error(self, errormsg)
            return

        if self._target and not isinstance(self._target, operand.ID):
            vld.error(self, "target must be an identifier")

    def output(self, printer):
        printer.printComment(self, separate=True)

        if self._target:
            self._target.output(printer)
            printer.output(" = ")

        printer.output(self.name())

        for op in (self._op1, self._op2, self._op3):
            if op:
                printer.output(" ")
                op.output(printer)

        printer.output("", nl=True)

    def _canonify(self, canonifier):
        for op in (self._op1, self._op2, self._op3, self._target):
            if op:
                op.canonify(canonifier)

    # Visitor support.
    def visit(self, visitor):
        visitor.visitPre(self)

        for op in (self._op1, self._op2, self._op3, self._target):
            if op:
                op.visit(visitor)

        visitor.visitPost(self)

class BlockingBase:
    """Base class for instructions/operators that can potentially block when
    executed. If so, execution yields back to the scheduler and the
    instruction will be executed again upon resuming.

    Currently, the instruction's functionality must be implemented as a call
    to C function; we might generalize that at some point though. The function
    must throw ``WouldBlock`` exception to signal blocking and return to its
    caller.

    Derived classes must override ~~cCall, and can override ~~cResult.

    See ~~Instruction for arguments.
    """
    def cCall(self, cg):
        """Returns the name of the C function to call for this instruction,
        and the calls arguments.

        Must be overridden by derived classes.

        cg: ~~CodeGen - The code generator to use.

        Returns: tuple (func, args) - *func* is the name of the C function, and
        *arg* a list of arguments to pass into it.
        """
        util.internal_error("cCall() not overridden")

    def cResult(self, cg, result):
        """Receives the C calls return value. It can, e.g., use
        ~~llvmStoreInTarget() to store the value. This function will only be
        called if the instruction has a target operand.

        Can be overridden by derived classes. The default implementation does
        nothing.

        cg: ~~CodeGen - The code generator to use.

        result: llvm.core.Value - The return value.
        """
        pass

    def blockingCanonify(self, canonifier):
        # We rewrite the instruction into the form
        #
        #    <instruction>
        #    jump new_block
        #  newblock:
        #    <...>
        #
        # This ensures that we can capture a continuation at newblock.
        canonifier.deleteCurrentInstruction()

        current_block = canonifier.currentTransformedBlock()
        current_block.addInstruction(self)

        new_block = block.Block(canonifier.currentFunction(), name=canonifier.makeUniqueLabel())
        new_block.setNext(current_block.next())
        current_block.setNext(new_block.name())
        canonifier.addTransformedBlock(new_block)

        self._succ = new_block

    def blockingCodegen(self, cg):
        def _result(cg, result):
            self.cResult(cg, result)

        def _cfunc(cg):
            return self.cCall(cg)

        cg.llvmCallCWithRetry(self._succ, self.cCall, result_func=_result if self.target() else None)

class BlockingInstruction(Instruction, BlockingBase):
    """Class for instructions that can potentially block when
    executed. See ~~BlockingBase for more information.

    Derived classes must override ~~cCall, and can override ~~cResult.

    See ~~Instruction for arguments.
    """
    def _canonify(self, canonifier):
        super(BlockingInstruction, self)._canonify(canonifier)
        self.blockingCanonify(canonifier)

    def _codegen(self, cg):
        self.blockingCodegen(cg)

class InstructionWithCallables(Instruction):
    """Class for instructions that can potentially add callables to the
    internal queue via the C run-time.

    See ~~Instruction for arguments.

    Note: The instruction must not be a terminator at the moment.
    """
    def canonify(self, canonifier):
        assert not self.signature().terminator()
        Instruction.canonify(self, canonifier)

        # We rewrite the instruction into the form
        #
        #    <instruction>
        #    call hlt::call_saved_callables()
        #
        #  newblock:
        #    <...>
        #
        # This ensures that we can capture a continuation at newblock.
        canonifier.deleteCurrentInstruction()

        current_block = canonifier.currentTransformedBlock()
        current_block.addInstruction(self)

        new_block = block.Block(canonifier.currentFunction(), name=canonifier.makeUniqueLabel())
        new_block.setNext(current_block.next())
        current_block.setNext(new_block.name())
        canonifier.addTransformedBlock(new_block)

        csc = canonifier.currentModule().scope().lookup("hlt::call_saved_callables")
        assert csc
        op1 = operand.ID(csc)
        op2 = operand.Constant(constant.Constant([], type.Tuple([])))
        op3 = operand.ID(id.Local(new_block.name(), type.Label()))

        call = instructions.flow.CallTailVoid(op1=op1, op2=op2, op3=op3)
        current_block.addInstruction(call)

class Operator(Instruction):
    """Class for instructions that are overloaded by their operands' types.
    While most HILTI instructions are tied to a particular type, *operators*
    are generic instructions that can operator on different types.

    To create a new operator do *not* derive directly from Operator but use
    the ~~operator decorator.

    op1: ~~Operand - The operator's first operand, or None if unused.
    op2: ~~Operand - The operator's second operand, or None if unused.
    op3: ~~Operand - The operator's third operand, or None if unused.
    target: ~~ID - The operator's target, or None if unused.
    location: ~~Location - A location to be associated with the operator.
    """
    def __init__(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(Operator, self).__init__(op1, op2, op3, target, location)

    ### Overridden from Node.

    def _validate(self, vld):
        super(Operator, self)._validate(vld)

        ops = _findOverloadedOperator(self)

        if len(ops) == 0:
            vld.error(self, "no matching implementation of overloaded operator found")

        if len(ops) > 1:
            vld.error(self,  "use of overloaded operator is ambigious, multiple matching implementations found:")

        self._callOps(ops, "_validate", vld)

    def _resolve(self, resolver):
        super(Operator, self)._resolve(resolver)
        ops = _findOverloadedOperator(self)
        if ops:
            self._callOps(ops, "_resolve", resolver)

    def _canonify(self, canonifier):
        super(Operator, self)._canonify(canonifier)
        ops = _findOverloadedOperator(self)
        assert ops
        self._callOps(ops, "_canonify", canonifier)

    def _codegen(self, cg):
        ops = _findOverloadedOperator(self)
        assert ops
        self._callOps(ops, "_codegen", cg)

        for op in ops:
            self._llvm_target = op._llvm_target

    ### Private.

    def _callOps(self, ops, method, arg):
        for o in ops:
            try:
                # We can't call o.resolve() here directly as that recurse
                # infinitly if the class doesn't override the implementation of
                # the method.
                o.__class__.__dict__[method](o, arg)
            except KeyError:
                pass

class BlockingOperator(Operator, BlockingBase):
    """Class for instructions that can potentially block when executed. See
    ~~BlockingBase for more information.

    Derived classes must override ~~cCall, and can override ~~cResult. Derived
    classes must also still override ~~canonify and ~~codegen, and in both
    cases forward directly to ~~blockingCanonify and ~~blockingCodegen,
    respectively. The latter is different than with ~~BlockingInstruction,
    which doesn't require that (though it doesn't hurt either). ~~canonify
    must also call ~~Instruction.canonify(!).

    See ~~Instruction for arguments.

    Note: The interface for derived classes insn't very nice but required due
    to the way we currently do the method lookups. Should find a better
    solution.
    """
    pass

from signature import *

def _make_ins_init(myclass):
    def ins_init(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(self.__class__, self).__init__(op1, op2, op3, target, location)

    ins_init.myclass = myclass
    return ins_init

def instruction(name, op1=None, op2=None, op3=None, target=None, callback=None, terminator=False, location=None):
    """A *decorater* for classes derived from ~~Instruction. The decorator
    defines the new instruction's ~~Signature. The arguments correpond to
    those of the ~~Signature constructor."""
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback, terminator)
        d = dict(ins.__dict__)
        d["__init__"] = _make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        _Instructions[name] = newclass
        return newclass

    return register

# Currently, "operator" is just an alias for "instruction".
operator = instruction
"""A *decorater* for classes derived from ~~Operator. The decorator
defines the new operators's ~~Signature. The arguments correpond to
those of the ~~Signature constructor."""

def overload(operator, op1, op2=None, op3=None, target=None):
    """A *decorater* for classes that provide type-specific overloading of an
    operator. The decorator defines the overloaded operator's ~~Signature. The
    arguments correpond to those of the ~~Signature constructor except for
    *operator* which must be a subclass of ~~Operator."""
    def register(ins):
        global _OverloadedOperators
        assert issubclass(operator, Operator)

        global _Instructions
        ins._signature = Signature(operator().name(), op1, op2, op3, target)
        d = dict(ins.__dict__)
        d["__init__"] = _make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)

        idx = operator.__name__
        try:
            _OverloadedOperators[idx] += [ins]
        except:
            _OverloadedOperators[idx] = [ins]

        return newclass

    return register

_Instructions = {}
_OverloadedOperators = {}

def getInstructions():
    """Returns a dictionary of instructions. More precisely, the function
    returns all classes decorated with either ~~instruction or ~~operator;
    these classes will be derived from ~~Instruction and represent a complete
    enumeration of all instructions provided by the HILTI language. The
    dictionary is indexed by the instruction name and maps the name to the
    ~~Instruction instance.

    Returns: dictionary of ~~Instruction-derived classes - The list of all
    instructions.
    """
    return _Instructions

def createInstruction(name, op1=None, op2=None, op3=None, target=None, location=None):
    """Instantiates a new instruction.

    name: The name of the instruction to be instantiated; i.e., the mnemnonic
    as defined by a ~~instruction decorator.

    op1: ~~Operand - The instruction's first operand, or None if unused.
    op2: ~~Operand - The instruction's second operand, or None if unused.
    op3: ~~Operand - The instruction's third operand, or None if unused.
    target: ~~ID - The instruction's target, or None if unused.
    location: ~~Location - A location to be associated with the instruction.
    """
    try:
        i = _Instructions[name](op1, op2, op3, target, location)
    except KeyError:
        return None

    return i

def _findOverloadedOperator(op):
    """Returns the type-specific version(s) of an overloaded operator. Based on
    an instance of ~~Operator, it will search all type-specific
    implementations (i.e., all ~~OverloadedOperators) and return the matching
    ones.

    op: ~~Operator - The operator for which to return the type-specific version.

    Returns: list of ~~OverloadedOperator - The list of matching operater
    implementations.
    """

    matches = []

    try:
        return op._cached
    except AttributeError:
        pass

    try:
        for o in _OverloadedOperators[op.__class__.__name__]:
            (success, errormsg) = o._signature.matchWithInstruction(op)
            if success:
                ins = o(op1=op.op1(), op2=op.op2(), op3=op.op3(), target=op.target(), location=op.location())
                matches += [ins]
    except KeyError:
        pass

    op._cached = matches
    return matches

