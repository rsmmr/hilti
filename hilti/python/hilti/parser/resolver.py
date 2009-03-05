# $Id$

import sys

from hilti.core import *
from hilti import instructions

class Resolver(visitor.Visitor):
    """
    Performs an initial pass over an |ast|, preparing for later
    stages. The Resolver performs the following tasks:
    
    - It resolves all ~~IDOperands. The parser initially sets all
      ~~IDOperands to type ~~Unknown, and the resolver goes through
      the |ast| and tries to resolve them to their actual type.

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
     
      If none of these apply, integer constants are set to type
      ``int64``.
      
    Note: Methods in this class need to be careful with their input as it will
    not have been validated by the ~~Checker when they are called; that's
    going to happen later. However, any errors the methods detect, they can
    silently ignore as it's the checker duty to report them (though one should
    make sure that that is indeed the case).
    
    Todo: This class now does more than just resolving. Should rename it. 
    """
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
        
        if not isinstance(tuple, instruction.TupleOperand):
            return

        try:
            args = tuple.value()
            types = [id.type() for id in function.type().args()]
            
            for i in range(len(args)):
                arg = args[i]
                
                if not isinstance(arg.type(), type.Integer) or arg.type().width() != 0:
                    continue
                
                if isinstance(types[i], type.Integer):
                    self._setIntWidth(arg, types[i].width())
                    
                if isinstance(types[i], type.Any):
                    self._setIntWidth(arg, 64)
                    
        except (IndexError, TypeError, AttributeError):
            # Can happen if the call gives the wrong number of arguments. That
            # hasn't been checked yet but will be reported later. 
            return

    def adaptIntReturn(self, function, op):
        """Derives a width for integer constants in return statements. The
        function modifies the given operand in place. 
        
        function: ~~Function - The function being returned from. 
        tuple: ~~ConstOperand - The return's operand.
        """
        
        if not isinstance(op, instruction.ConstOperand):
            return
        
        if not isinstance(op.type(), type.Integer) or op.type().width() > 0:
            return
        
        if not isinstance(function.type().resultType(), type.Integer):
            return 

        self._setIntWidth(op, function.type().resultType().width())

    def adaptIntConsts(self, i):
        """Derives a width for integer constants based on further integer
        operands. See ~~Resolver for details. The function modifies the given
        instruction in place.
        
        i: ~~Instruction - The instruction to operate on.
        """
        def adaptIntType(op, width):
            
            if not op:
                return
            
            if not isinstance(op, instruction.ConstOperand) or not isinstance(op.type(), type.Integer):
                return
            
            self._setIntWidth(op, width)

        # Determine largest width across all int operands.
        widths = [op.type().width() for op in (i.op1(), i.op2(), i.op3(), i.target()) if op and isinstance(op.type(), type.Integer)]
        maxwidth = max(widths) if widths else 0
    
        if not maxwidth:
            # Either no integer operand at all, or none with a width. As the
            # latter can only be constants, we pick a default.
            maxwidth = 64
            
        adaptIntType(i.op1(), maxwidth)
        adaptIntType(i.op2(), maxwidth)
        adaptIntType(i.op3(), maxwidth)
        
    def resolveInstrOperands(self, i):
        """Attempts to resolve all IDs in an instruction. ID's which cannot be
        resolved are left with type ~~Unknown. The given instruction is
        modified in place.
        
        i: ~~Instruction: The instruction to resolve all IDs in.
        """
        def resolveOp(op, tag):
            if not op:
                return

            if isinstance(op, instruction.TupleOperand):
                # Look up all operands inside the tuple recursively.
                for o in op.value():
                    resolveOp(o, tag)
                return
        
            if not isinstance(op, instruction.IDOperand):
                return
        
            if not isinstance(op.type(), type.Unknown):
                return

            ident = self.resolveID(op.value().name(), tag, i)
            if not ident:
                return 
        
            assert not isinstance(ident.type(), type.Unknown)
            op.setID(ident)
            
        resolveOp(i.op1(), "operand 1")
        resolveOp(i.op2(), "operand 2")
        resolveOp(i.op3(), "operand 3")
        resolveOp(i.target(), "target")
    
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

@resolver.when(instruction.Instruction)
def _(self, i):
    self.resolveInstrOperands(i) # need to do this first

    if isinstance(i, instructions.flow.Call):
        func = self._module.lookupIDVal(i.op1().value().name())
        if func:
            self.applyDefaultParams(func, i.op2())
            self.adaptIntParams(func, i.op2())

    if isinstance(i, instructions.flow.ReturnResult):
        self.adaptIntReturn(self._function, i.op1())
        
    self.adaptIntConsts(i)

    
    
    
