# $Id$

import sys

from hilti.core import *
from hilti import instructions

class Resolver(visitor.Visitor):
    """
    Performs an initial pass over an |ast|, preparing for later
    stages. The Resolver performs the following tasks:
    
    - It resolves all ~~IDOperands. The parser initially sets all ~~IDOperands
      to type ~~Unknown, and the resolver goes through the |ast| and tries to
      resolve them to their actual type. If an ~~IDOperand turns out to refer
      to a ~~TypeDeclType, it's turned into a ~~TypeOperand. If an ~~IDOperand
      refers to an ~~Enum or ~~Bitset label, it's turned into a ConstOperand.

    - Resolves the base classes of exception types. The parser intially sets
      all base classes to a string with the name of the exception. The
      resolvers looks up the ID and resets it to be the type itself. 

    - If a function ~~Call is lacking arguments but the called
      function's type provides defaults, we add corresponding
      ConstOperands to the call's parameter tuple.
      
    - Assigns widths to integer constants, as follows:
    
      * If an integer constant is an arguments inside a function
        ~~Call, it gets its width from the prototype of the called
        function. If the prototype specified ~~Any for the
        corresponding argument, it gets type ``int64``.
        
      * If an integer constant appears in a ~~ReturnResult
        statement, it gets its width from the return type of the
        function it's in.
     
      * If an instruction has multiple integer operands, all integer
        constants are set to the largest widths across them.
        
      These rules are recursively applied to integers in tuples, as
      appropiate.  If none of them applies, integer constants are
      set to type ``int64``.

    - Resolves the *fmt* attribute of ~~Overlay fields (which is initially a
      string) to the numerical value corresponding to the ``Hilti::Packed``
      value; and assign a width to all potential integer field arguments.
      
    Note: Methods in this class need to be careful with their input as it will
    not have been validated by the ~~checker when they are called; that's
    going to happen later. However, any errors the methods detect, they can
    silently ignore as it's the checker duty to report them (though one should
    make sure that that is indeed the case).
    
    Todo: This class now does more than just resolving. Should rename it. 
    Furthermore, we should add more sophisticated derivation of int widhts
    inside tuples; currently they are always set to 64bit.
    """
    
    Debug = False
    
    def __init__(self):
        super(Resolver, self).__init__()
        self.reset()
        
    def reset(self):
        """Resets the internal state. After resetting, the object can be used
        to resolve another |ast|.
        """
        self._errors = 0

    def resolveOperands(self, ast):
        """Attempts to resolve all IDOperands in |ast|. If it cannot resolve
        an identifier, it reports an error message via ~~error.
        
        ast: ~~Node - The root of the |ast| to resolve. 
        
        Returns: int - Number of unresolved identifiers. 
        """
        self.reset()
        self.visit(ast)
        return self._errors
    
    def error(self, message, obj):
        """Reports an error message to the user.

        message: string - The error message to report.
        
        obj: object - Provides the location information to include in the
        message. *obj* must have a method *location()* returning a ~~Location
        object. Usually, *obj* is just the object currently being visited.
        """
        self._errors += 1
        util.error(message, context=obj.location(), fatal=False)        

    def resolveID(self, name, tag, obj):
        """Resolves the name to an ID. The methods looks up the ID
        in the current function's scope (if any) as well as in the
        current module's scope. If the ID is not found, reports an
        error message. 
        
        name: string - The name of the ID to resolve.
        
        tag: string - Additional string to include into any error message.
        
        obj: object - Provides the location information to include in the
        message. *obj* must have a method *location()* returning a ~~Location
        object. Usually, *obj* is just the object currently being visited.
        
        Returns: ~~ID - The ID the name resolved to, or None if not
        found.
        """

        ident = self._function.lookupID(name)
        if not ident:
            ident = self._function.type().getArg(name)
            if not ident:
                ident = self._module.lookupID(name)
                if not ident:
                    self.error("unknown identifier %s in %s" % (name, tag), obj)
                    return None
                
        return ident

    def applyDefaultParams(self, function, tuple):
        """Adds missing arguments to a call's parameter tuple if
        default values are known. The function modifies the given
        argument tuple in place.
        
        function: ~~Function - The function which is called.
        tuple: ~~TupleOperand - The call's argument operand.
        """
        
        if not isinstance(tuple, instruction.TupleOperand):
            return
        
        try:
            args = tuple.value()
            defaults = [t[1] for t in function.type().argsWithDefaults()]
        
            if len(args) >= len(defaults):
                return
            
            new_args = args
        
            for i in range(len(args), len(defaults)):
                d = defaults[i]
                if d:
                    new_args += [d]
                    
            tuple.setTuple(new_args)
        
        except (IndexError, TypeError, AttributeError):
            # Can happen if the call gives wrong arguments. That hasn't been
            # checked yet but will be reported later. 
            return

    def _setIntWidth(self, op, width):
        if op.type().width() > 0:
            # Already set, don't change.
            return 

        (succ, t) = type.getHiltiType("int", [width])
        assert(succ)
        const = constant.Constant(op.value(), t)
        op.setConstant(instruction.ConstOperand(const))
        
    def adaptIntParams(self, function, tuple):
        """Derives a width for constant integer parameters to a function call.
        The function modifies the given argument tuple in place. 
        
        function: ~~Function - The function being called.
        tuple: ~~TupleOperand - The call's argument operand.
        """

        def _adaptSingle(tuple, args, types):
            try:
                for i in range(len(args)):
                    arg = args[i]

                    if isinstance(arg, instruction.TupleOperand):
                        # Recurse
                        _adaptSingle(arg, arg.value(), types[i].types())
                        tuple.setTuple(tuple.value()) # update types
                        return
                    
                    if not isinstance(arg, instruction.ConstOperand):
                        continue
                    
                    if not isinstance(arg.type(), type.Integer):
                        continue
                    
                    if arg.type().width() != 0:
                        continue
    
                    if types and isinstance(types[i], type.Integer):
                        self._setIntWidth(arg, types[i].width())
                        
                    if types and isinstance(types[i], type.Any):
                        self._setIntWidth(arg, 64)
                        
                    if arg.type().width() == 0:
                        self._setIntWidth(arg, 64)
                        
                    tuple.setTuple(tuple.value()) # update types
                        
            except (IndexError, TypeError, AttributeError):
                # Can happen if the call gives the wrong number of arguments. That
                # hasn't been checked yet but will be reported later. 
                pass

        args = tuple.value()
        types = [id.type() for id in function.type().args()]
        _adaptSingle(tuple, args, types)
        
        
    def adaptIntReturn(self, function, op):
        """Derives a width for integer constants in return statements. The
        function modifies the given operand in place. 
        
        function: ~~Function - The function being returned from. 
        tuple: ~~ConstOperand - The return's operand.
        """

        def adaptSingle(op, rt):
            
            if isinstance(op, instruction.TupleOperand):
                for i in range(len(op.value())):
                    adaptSingle(op.value()[i], rt.types()[i])
            
            if not isinstance(op, instruction.ConstOperand):
                return
        
            if not isinstance(op.type(), type.Integer) or op.type().width() > 0:
                return
        
            if not isinstance(rt, type.Integer):
                return 

            self._setIntWidth(op, rt.width())

        adaptSingle(op, function.type().resultType())
            
    def _adaptIntValues(self, ops):
        # Adapts the list *ops* of integer operands so that all constants
        # without a width get the width of the widest other integer operands. 
        # The list can contain *None*, which are ignored.

        def adaptIntType(op, ty, sig, width):
            if not op or not isinstance(op, instruction.ConstOperand) or not isinstance(ty, type.Integer):
                return

            # Instruction's signature has preference.
            if sig and isinstance(sig, type.Integer) and sig.width() > 0:
                width = sig.width()
            
            self._setIntWidth(op, width)

        widths = [ty.width() for (op, ty, sig) in ops if ty and isinstance(ty, type.Integer)]
        maxwidth = max(widths) if widths else 0
        
        if not maxwidth:
            # Either no integer operand at all, or none with a width. As the
            # latter can only be constants, we pick a default.
            maxwidth = 64
            
        for (op, ty, sig) in ops:
            adaptIntType(op, ty, sig, maxwidth)

    def adaptIntConsts(self, i):
        """Derives a width for integer constants based on further integer
        operands. See ~~Resolver for details. The function modifies the given
        instruction in place.
        
        i: ~~Instruction - The instruction to operate on.
        """

        all_ops = []
        
        if i.op1():
            all_ops += [(i.op1(), i.op1().type(), i.signature().op1())]
            
        if i.op2():
            all_ops += [(i.op2(), i.op2().type(), i.signature().op2())]
            
        if i.op3():
            all_ops += [(i.op3(), i.op3().type(), i.signature().op3())]
            
        if i.target():
            all_ops += [(i.target(), i.target().type(), i.signature().target())]
        
        # First do all direct integer constants. 
        self._adaptIntValues(all_ops)
        
        # Then do all integer constants inside tuples.

        ops = []
        
        for (op, ty, sig) in all_ops:
            
            if not op:
                continue
            
            if isinstance(op, instruction.TupleOperand):
                types = ty.types()
                vals = op.value()
                sigs = sig.types() if sig and isinstance(sig, type.Type) else [None] * len(types)
                
            elif isinstance(op, instruction.IDOperand) and isinstance(ty, type.Tuple):
                types = ty.types()
                vals = [None] * len(types)
                sigs = sig.types() if sig and isinstance(sig, type.Type) else [None] * len(types)
                
            else:
                continue

            ops += [zip(vals, types, sigs)]

        tuples = zip(*ops)
            
        for t in tuples:
            self._adaptIntValues(t)

        # Update types
        for (op, ty, sig) in all_ops:
            if op and isinstance(op, instruction.TupleOperand):
                op.setTuple(op.value())
            
    def resolveInstrOperands(self, i):
        """Attempts to resolve all IDs in an instruction. ID's which cannot be
        resolved are left with type ~~Unknown. The given instruction is
        modified in place.
        
        i: ~~Instruction: The instruction to resolve all IDs in.
        """
        def resolveOp(op, tag):
            if not op:
                return (False, None)

            if isinstance(op, instruction.TupleOperand):
                # Look up all operands inside the tuple recursively.
                new_ops = []
                for o in op.value():
                    (replace, on) = resolveOp(o, tag)
                    new_ops += [on if replace else o]
                    
                op.setTuple(new_ops) # Update type
                return (False, None)
        
            if not isinstance(op, instruction.IDOperand):
                return (False, None)
        
            if not isinstance(op.type(), type.Unknown):
                return (False, None)

            ident = self.resolveID(op.value().name(), tag, i)
            
            if not ident:
                return (False, None)

            assert not isinstance(ident.type(), type.Unknown)
            
            if isinstance(ident.type(), type.TypeDeclType):
                return (True, instruction.TypeOperand(ident.type().declType()))

            if (isinstance(ident.type(), type.Enum) or isinstance(ident.type(), type.Bitset) ) and ident.role() == id.Role.CONST:
                label = ident.name() 
                j = label.find("::")
                assert j > 0
                label = label[j+2:]
                const = constant.Constant(ident.type().labels()[label], ident.type())
                return (True, instruction.ConstOperand(const))
                
            op.setID(ident)
            return (False, None)
            
        (replace, op) = resolveOp(i.op1(), "operand 1")
        if replace:
            i.setOp1(op)
        
        (replace, op) = resolveOp(i.op2(), "operand 2")
        if replace:
            i.setOp2(op)
            
        (replace, op) = resolveOp(i.op3(), "operand 3")
        if replace:
            i.setOp3(op)

        (replace, op) = resolveOp(i.target(), "target")
        if replace:
            i.setTarget(op)
            
    def _debugInstruction(self, i, tag):

        if not self.Debug:
            return
        
        def fmtOpTypes(ops):
            return " | ".join(["%s=%s" % (tag, (op.type() if op else "-")) for (tag, op) in ops])
        
        types = fmtOpTypes([("target", i.target()), ("op1", i.op1()), ("op2", i.op2()), ("op3", i.op3())])
        util.debug("resolver after %s: %s" % (tag, str(i)))
        util.debug("")
        util.debug("    %s" % types)
        util.debug("")
        
    def resolveOverlayFields(self, t, ident):
        for f in t.fields():
            if isinstance(f.fmt(), str):
                label = f.fmt()
                v = self._module.lookupID(label)
        
                if not v or not isinstance(v.type(), type.Enum):
                    self.error("%s is not a valid enum label" % label, ident)
                    continue
        
                hp = self._module.lookupID("Hilti::Packed")
                    
                if not hp or not v.type() is hp.type().declType():
                    self.error("%s is not a label of type Hilti::Packed" % label, t)
                    continue
                    
                f._fmt = self._module.lookupIDVal(label)
                
            if f.arg():
                # Adapt integer constants' width in tuples. FIXME: We need one
                # generic, recursive adapt-all-ints-operand function. 
                if isinstance(f.arg().type(), type.Tuple):
                    self._adaptIntValues([(op, op.type(), None) for op in f.arg().value()])
                    f.arg().setTuple(f.arg().value()) # update types

    def resolveExceptionBase(self, t, ident):
        base = t.baseClass()
        
        if not base or isinstance(base, type.HiltiType):
            # Already resolved (or not set). 
            return
        
        baseid = self._module.lookupID(base)
        if not baseid:
            self.error("unknow ID %s" % base, ident)
            return
            
        t.setBaseClass(baseid.type().declType())
                    
resolver = Resolver()

##################################################################################

@resolver.pre(module.Module)
def _(self, m):
    self._module = m

@resolver.post(module.Module)
def _(self, m):
    self._module = None
    
@resolver.pre(function.Function)
def _(self, f):
    self._function = f
        
@resolver.post(function.Function)
def _(self, f):
    self._function = None

@resolver.when(id.ID, type.TypeDeclType)
def _(self, id):
    t = id.type().declType()
    if isinstance(t, type.Overlay):
        self.resolveOverlayFields(t, id)
        
    if isinstance(t, type.Exception):
        self.resolveExceptionBase(t, id)
    
@resolver.when(instruction.Instruction)
def _(self, i):
    self._debugInstruction(i, "starting on instruction")
    self.resolveInstrOperands(i) # need to do this first
    self._debugInstruction(i, "resolveInstrOperands()")

    if isinstance(i, instructions.flow.Call):
        func = self._module.lookupIDVal(i.op1().value())
        if func:
            self.applyDefaultParams(func, i.op2())
            self._debugInstruction(i, "applyDefaultParams()")
            self.adaptIntParams(func, i.op2())
            self._debugInstruction(i, "applyIntParams()")

    if isinstance(i, instructions.flow.ReturnResult):
        self.adaptIntReturn(self._function, i.op1())
        self._debugInstruction(i, "applyIntReturn()")
        
    self.adaptIntConsts(i)
    self._debugInstruction(i, "adaptIntConsts()")
    
    if self.Debug:
        util.debug("---------------------")
