# $Id$
#
# The function type.

import binpac.ast as ast
import binpac.type as type
import binpac.expr as expr
import binpac.id as id
import binpac.util as util
import binpac.operator as operator
import binpac.constant as constant

import hilti.type

@type.pac(None)
class Function(type.Type):
    """Type for functions. 
    
    args: list of either ~~id.Parameter or of (~~id.Parameter, ~~Expression)
    tuples - The function's arguments, with the second tuple element being
    optional default values in the latter case. The default expression can
    also be set to *None* for no default.
    
    resultt - ~~Type - The type of the function's return value (~~Void for none). 
    """
    def __init__(self, args, resultt, location=None):
        super(type.Function, self).__init__(location=location)
        
        if args and isinstance(args[0], id.Parameter):
            self._ids = args
            self._defaults = [None] * len(args)
        else:
            self._ids = [i for (i, default) in args]
            self._defaults = [default for (i, default) in args]
            
        self._result = resultt

    def args(self):
        """Returns the functions's arguments.
        
        Returns: list of ~~id.Parameter - The function's arguments. 
        """
        return self._ids

    def defaults(self):
        """Returns the functions's default expressions.
        
        Returns: list of ~~Expression - The function's defaults. 
        """
        return self._defaults
    
    def argsWithDefaults(self):
        """Returns the functions's arguments with any default values.
        
        Returns: list of (~~id.Parameter, default) - The function's arguments
        with their default values. If there's no default for an arguments,
        *default* will be None.
        """
        return zip(self._ids, self._defaults)
    
    def getArg(self, name):
        """Returns the named function argument.
        
        Returns: ~~id.Parameter - The function's argument with the given name,
        or None if there is no such arguments. 
        """
        for id in self._ids:
            if id.name() == name:
                return id
            
        return None
    
    def resultType(self):
        """Returns the functions's result type.
        
        Returns: ~~Type - The function's result. 
        """
        return self._result

    ### Methods overidden from ast.Node.
    
    def validate(self, vld):
        for i in self._ids:
            util.check_class(i, id.Parameter, "type.Function.validate")
            i.validate(vld)
            
        for d in self._defaults:
            if d: 
                if d.isInit():
                    vld.error(self, "function argument's default value cannot be used as an initializer")
                
                d.validate(vld)

    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        for i in self._ids:
            i.resolve(resolver)
            
        for d in self._defaults:
            if d:
                d.resolve(resolver)
            
        self._result = self._result.resolve(resolver)
        
        return self

    def doSimplify(self):
        self._defaults = [d.simplify() for d in self._defaults]
        return self
        
    def pac(self, printer):
        printer.output("<function-type pac2 not implemented>")
        
    ### Methods overidden from Type.
        
    def hiltiType(self, cg):
        result = self._result.hiltiType()
        args = [a.hiltiType() for a in self._args]
        defaults = [(e.hiltiInit(cg) if e else None) for e in self._defaults]
        return hilti.type.Function(zip(args, defaults), result)

    def name(self):
        return "function" 
    
@type.pac(None)
class OverloadedFunction(type.Type):
    """Type for overloaded functions, that is functions with more than one
    implementation between which choose depending on giving arguments. 
    
    resultt - ~~Type - The type of the function's return value (~~Void for
    none). All implementations of the overloaded function must return the same
    type. 
    """
    def __init__(self, resultt, location=None):
        super(type.OverloadedFunction, self).__init__(location=location)
        self._result = resultt

    def resultType(self):
        """Returns the functions's result type.
        
        Returns: ~~Type - The function's result. 
        """
        return self._result

    ### Methods overidden from ast.Node.
    
    def validate(self, vld):
        self._result.validate(vld)
        
    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        self._result = self._result.resolve(resolver)
        return self

    def pac(self, printer):
        printer.output("<overloaded function pac not implemented>")
        
    ### Methods overidden from Type.
        
    def hiltiType(self, cg):
        util.internal_error("hiltiType() must not be called for type.OverloadedFunction")
        
    def name(self):
        return "overloaded function"     
    
class Function(ast.Node):
    """Base class for function implementations.
    
    ty: ~~Function - The type of the function.
    
    ident: ~~id.Function - The ID to which the function is bound. *id* can be
    initially None, but must be set via ~~setID before any other methods are
    used.
    
    location: ~~Location - A location to be associated with the function. 
    
    Note: Instances of this class should be used only within a containing
    ~~OverloadedFunction instance. 
    """
    
    def __init__(self, ty, location=None):
        assert isinstance(ty, type.Function)
        super(Function, self).__init__(location)
        self._type = ty
        self._parent = None
        
    def type(self):
        """Returns the type of the function. 
        
        Returns: ~~type.Function` - The type of the function.
        """
        return self._type

    def resultType(self):
        """Returns the functions's result type. This is for convenience only
        and just forwards to the corresponding ~~type.Function's method.
        
        Returns: ~~Type - The function's result. 
        """
        return self._type.resultType()
    
    def parent(self): 
        """Returns the function's overloaded parent function.  
        
        Returns: ~~OverloadedFunction - The parent function or None if not set. 
        """
        return self._parent
    
    def setParent(self, p): 
        """Sets the function's overloaded parent function.
        
        id: ~~OverloadedFunction  - The parent function.
        """
        self._parent = p

    ### Methods overidden from ast.Node.

    def validate(self, vld):
        self._type.validate(vld)
        
    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        self._type = self._type.resolve(resolver)
        return self

    ### Methods for derived classes to override.
    
    def evaluate(self, cg):
        """
        Generates the code implementating the function.
        
        cg: ~~CodeGen - The code generator to use. 
        """
        util.internal_error("Function.codegen not overidden for %s" % self.__class__)
    
    def hiltiCall(self, cg, args):
        """Generates code to call the function.
        
        cg: ~~CodeGen - The code generator to use. 
        
        args: list of Expression - The function arguments.
        
        Returns: ~~Expression or None - The function's result if it has any.
        """
        util.internal_error("Function.hiltiCall not overidden for %s" % self.__class__)

        
class HILTIFunction(Function):
    """Class for built-in function implemented in HILTI code. These functions
    must be declated in an ``*.hlt`` file that gets compiled with the current
    module.
    
    ty: ~~Function - The type of the function.
    
    hltname: string - The name of the HILTI function to be called.
    
    location: ~~Location - A location to be associated with the function. 
    """
    def __init__(self, ty, hltname, location=None):
        super(HILTIFunction, self).__init__(ty, location=location)
        self._hltname = hltname
        
    ### Methods overidden from ast.Node.

    def pac(self, printer):
        printer.output("<HILTIFunction.pac2 not implemented>")
        
    ### Methods overidden from Function.
    
    def hiltiCall(self, cg, args):
        builder = cg.builder()
        id = hilti.id.Unknown(self._hltname, cg.moduleBuilder().module().scope())
        tmp = cg.functionBuilder().addLocal("__result", self.resultType().hiltiType(cg))
        builder.call(tmp, hilti.operand.ID(id), builder.tupleOp([a.evaluate(cg) for a in args]))
        return tmp

class OverloadedFunction(ast.Node):
    """Class for an overloaded function, that is a function with multiple
    implemenations between we choose by the types of their arguments. 
    
    ty: ~~type.OverloadedFunction - The type of the function.
    
    ident: ~~id.Function - The ID to which the function is bound. *id* can be
    initially None, but must be set via ~~setID before any other methods are
    used.
    
    location: ~~Location - A location to be associated with the function. 
    """
    
    def __init__(self, ty, ident, location=None):
        super(OverloadedFunction, self).__init__(location)
        self._type = ty
        self.setID(ident)
        
        self._funcs = []

    def addFunction(self, func):
        """Adds a function implementation.
        
        func: ~~Function - The function to add.
        """
        self._funcs += [func]
        func.setParent(self)
        
    def functions():
        """Returns all functions part of this overloaded function.
        
        Returns: list of ~~Function - The functions. 
        """
        return self._funcs
        
    def name(self):
        """Returns the function's name.
        
        Returns: string - The name of the function.
        """
        return self._id.name()

    def type(self):
        """Returns the type of the function. 
        
        Returns: ~~type.Function` - The type of the function.
        """
        return self._type

    def resultType(self):
        """Returns the functions's result type. This is for convenience only
        and just forwards to the corresponding ~~type.Function's method.
        
        Returns: ~~Type - The function's result. 
        """
        return self._type.resultType()
    
    def id(self): 
        """Returns the function's ID. 
        
        Returns: ~~id.Function - The ID to which the function is bound. 
        """
        return self._id
    
    def setID(self, i): 
        """Sets the ID to which the function is bound.
        
        id: ~~id.Function  - The ID.
        """
        if i:
            util.check_class(i, id.Function, "setID")
            
        self._id = i

    def matchFunctions(self, args):
        """Returns the functions matching with a given set of arguments.
        
        Returns: list of ~~Function - The overloaded functions matching the
        arguments.
        """
        funcs = []
        
        for func in self._funcs:
            proto = [a.type() for a in func.type().args()]
            defaults = func.type().defaults()
            
            if len(proto) < len(args):
                continue
            
            for i in range(len(proto)):
                if i < len(args):
                    if not args[i].canCastTo(proto[i]):
                        break
                    
                else:
                    if not defaults[i]:
                        break
                    
            else:
                # Match.
                funcs += [func]
                
        return funcs

    ### Methods overidden from ast.Node.

    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        self._type = self._type.resolve(resolver)
        self._id.resolve(resolver)
        
        for func in self._funcs:
            func.resolve(resolver)
            
        return self

    def validate(self, vld):
        self._type.validate(vld)
        # Don't validate ID to avoid recursion.
        
        if not self._funcs:
            util._internal_error("function ID without any implementation")
            
        for func in self._funcs:
            if func.resultType() != self.resultType():
                    vld.error(func, "return value of overloaded function does not match")
            
            func.validate(vld)

    def codegen(self, cg):
        for f in self._funcs:
            f.codegen(cg)
            
@operator.Call(type.OverloadedFunction, operator.Any())
class Call:
    def type(name, args):
        return name.type().resultType()

    def validate(vld, name, args):
        
        if not isinstance(name, expr.Name):
            vld.error(name, "function call must use static name")
            
        fid = vld.currentModule().scope().lookupID(name.name())
        if not fid:
            vld.error(name, "unknown function %s" % name.name())
        
        if not isinstance(fid, id.Function):
            vld.error(name, "%s is not a function" % name.name())
            
        funcs = fid.function().matchFunctions(args)
        
        if len(funcs) == 0:
            vld.error(name, "no matching function found")
            
        if len(funcs) > 1:
            vld.error(name, "ambigious function call")
        
    def evaluate(cg, name, args):
        assert isinstance(name, expr.Name)
        fid = cg.currentModule().scope().lookupID(name.name())
        assert fid
        
        funcs = fid.function().matchFunctions(args)        
        assert funcs and len(funcs) == 1
        return funcs[0].hiltiCall(cg, args)
        
    
