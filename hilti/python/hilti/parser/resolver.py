# $Id$

import sys

from hilti.core import *

class Resolver(visitor.Visitor):
    """Resolves all IDOperands in an |ast|. The parser initially sets all
    ~~IDOperands to type ~~Unknown. This visitor goes through the |ast| and
    tries to resolve them to their actual type.
    
    In addition, assigns widths to integer constants by taking the
    largest widths of all - # other integer operands/target.
    
    Todo: This class now does more than just resolving. Should
    rename it. 
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
                    self.error(i, "unknown identifier %s in %s" % (name, tag))
                    return None
                
        return ident
    
    def _resolveOperands(self, i):
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
    
    def _adaptIntConsts(self, i):
        def adaptIntType(op):
            if op and isinstance(op, instruction.ConstOperand) and isinstance(op.type(), type.Integer):
                val = op.value()
                const = constant.Constant(val, type.Integer(maxwidth))
                op.setConstant(instruction.ConstOperand(const))

    
        widths = [op.type().width() for op in (i.op1(), i.op2(), i.op3(), i.target()) if op and isinstance(op.type(), type.Integer)]
    
        maxwidth = max(widths) if widths else 0
    
        if not maxwidth:
            # Either no integer operand at all, or none with a width. As the
            # latter can only be constants, we pick a reasonable default.
            maxwidth = 32
            
        adaptIntType(i.op1())
        adaptIntType(i.op2())
        adaptIntType(i.op3())
        
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
    self._resolveOperands(i) # need to do this first
    self._adaptIntConsts(i)

    
    
    
