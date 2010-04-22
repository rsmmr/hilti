#! $Id$ 
#
# BinPAC++ expressions.

import type
import ast
import grammar
import operator
import constant
import id
import binpac.util as util

import hilti.operand

class Expression(ast.Node):
    """Base class for all expression objects.

    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Expression, self).__init__(location)

    def canCastTo(self, dsttype):
        """Returns whether the expression can be cast to a given target type.
        If *dsttype* is of the same type as that of the expression, the result
        is always True. 
        
        *dstype*: ~~Type - The target type.
        
        Returns: bool - True if the expression can be casted. 
        """
        
        if self.isConst():
            const = self.simplify()
            # This will be a constant now
            assert isinstance(const, Constant)
            
            # We just try the case. 
            try:
                operator.castConstTo(const, dsttype)
                return True # It worked.
            except operator.CastError:
                return False # It did not.
            
        else:
            return operator.canCastNonConstExprTo(self, dsttype)
        
    def castTo(self, dsttype, cg=None):
        """Casts the expression to a differenent type. It's ok if *dsttype* is
        the same as the expressions type, the method will be a no-op then. 
        
        *dsttype*: ~~Type - The type to cast the expression into. 
        
        *cg*: ~~CodeGen - The current code generator; can be None if the
        expression is constant. 
        
        Returns: ~~hilti.operand.Operand - The resulting  expression of the target type
        with the casted expression.
        
        Throws: operator.CastError - If the conversion is not possible.  This
        will only happen if ~~canCastTo returns *False*.
        """
        if not self.canCastTo(dsttype):
            raise operator.CastError, "cannot convert type %s to type %s" % (self.type(), dsttype)

        if self.isConst():
            const = self.simplify()
            # This will be a constant now.
            assert isinstance(const, Constant)
            try:
                const = operator.castConstTo(const, dsttype)
                return const
            except operator.CastError:
                # Can't happen because canCastTo() returned true ...
                util.internal_error("unexpected cast error from %s to %s" % (self.type(), dsttype()))
            
        else:
            assert cg 
            return operator.castNonConstExprTo(cg, self, dsttype)
    
    def isConst(self):
        """Returns true if the expression evaluates to a constant value.
        
        Returns: bool - Boolean indicating whether the expression is a constant.
        """
        if isinstance(self, Constant):
            return True
        
        # Test if we can fold us into a constant.
        if isinstance(self.simplify(), Constant):
            return True
        
        return False

    def isInit(self):
        """Returns true if the expression evaluates to a value that can be
        used as the initialization value for a HILTI object. 

        Can be overidden by derived classes. The default implementation always
        returns False.
        
        Returns: bool - Boolean indicating whether the expression is an
        initialization value."""
        if isinstance(self, Ctor):
            return True
        
        return self.isConst()
    
    def hiltiInit(self, cg):
        """Returns a HILTI operand suitable to initialize a HILTI variable
        with the value of the expression. This method must only be called if
        ~~isInit indicates that the expression can be used in this fashio.
        
        Returns: hilti.operand.Operand - The initialization value. 
        """
        return self.evaluate(cg)
        
    ### Methods for derived classes to override.    
    
    def type(self):
        """Returns the type to which the expression evaluates to.

        Must be overidden by derived classes.
        
        Returns: ~~Type - The type.
        """
        util.internal_error("Expression::type not overridden in %s", self.__class__)

    def evaluate(self, cg):
        """Generates code to evaluate the expression.
 
        Must be overidden by derived classes. 
 
        *cg*: ~~CodeGen - The current code generator.
        
        Returns: ~~hilti.operand.Operand - An operand with
        the value of the evaluated expression.
        """
        util.internal_error("Expression::evaluate not overridden in %s", self.__class__)

    def __str__(self):
        return "<generic expression>"

class Assignable(Expression):
    """Base class for all expression objects to which one can assign values.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Assignable, self).__init__(location)
    
    def assign(self, cg, rhs):
        """Generates code to assign a value to the expression.
        
        Must be overridden by derived classes.
        
        *cg*: ~~CodeGen - The current code generator.
        *rhs* hilti.operand.Operand - The value to assign.
        """
        util.internal_error("Assignable::assign not overridden in %s", self.__class__)
    
class Overloaded(Expression):
    """Class for expressions overloaded by the type of their operands. 

    op: string - The name of the operator; use one of the
    ``operator.Operator.*`` constants here.
    
    exprs: list of ~~Expression - The operand expressions.
    
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, op, exprs, location=None):
        super(Overloaded, self).__init__(location=location)
        self._op = op
        self._exprs = exprs

    def exprs(self):
        """Returns the expression's operands.
        
        Returns: list of ~~Expression - The operands.
        """
        return self._exprs

    ### Overidden from ast.Node.

    def resolve(self, resolver):
        for expr in self._exprs:
            if isinstance(expr, Expression):
                expr.resolve(resolver)

        if operator.typecheck(self._op, self._exprs):
            operator.resolve(self._op, resolver, self._exprs)
                
    def validate(self, vld):
        for expr in self._exprs:
            if isinstance(expr, Expression):
                expr.validate(vld)
            
        if not operator.typecheck(self._op, self._exprs):
            vld.error(self, "no match for overloaded operator")
            
        operator.validate(self._op, vld, self._exprs)

    def pac(self, printer):
        operator.pacOperator(printer, self._op, self._exprs)

    def simplify(self):
        self._exprs = [(e.simplify() if isinstance(e, ast.Node) else e) for e in self._exprs]
        expr = operator.simplify(self._op, self._exprs)
        return expr if expr else self
        
    ### Overidden from Expression.

    def type(self):
        t = operator.type(self._op, self._exprs)
        assert t
        return t

    def evaluate(self, cg):
        return operator.evaluate(self._op, cg, self._exprs)

    def assign(self, cg, rhs):
        return operator.assign(self._op, cg, self._exprs, rhs)
    
    def __str__(self):
        return "(%s %s)" % (self._op, " ".join([str(e) for e in self._exprs]))

class Constant(Expression):
    """A constant expression.
    
    const: ~~Constant - The constant.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, const, location=None):
        super(Constant, self).__init__(location=location)
        assert isinstance(const, constant.Constant)
        self._const = const
        
    def constant(self):
        """Returns the constant value.
        
        Returns: ~~Constant - The constant.
        """
        return self._const

    ### Overidden from ast.Node.
    
    def validate(self, vld):
        self._const.validate(vld)
    
    def pac(self, printer):
        self._const.pac(printer)

    ### Overidden from Expression.

    def type(self):
        return self._const.type()
    
    def evaluate(self, cg):
        c = self._const.type().hiltiConstant(cg, self._const)
        return hilti.operand.Constant(c)
    
    def __str__(self):
        return str(self._const.value())
    
class Ctor(Expression):
    """A constructor expression.
    
    value: any - The value of the ctor of a type-specific type.
    ty: ~~Type - The type of the expression.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, value, ty, location=None):
        super(Ctor, self).__init__(location=location)
        self._value = value
        self._type = ty
        
    def value(self):
        """Returns the value of the ctor expression.
        
        Returns: any - The value of the ctor of a type-specific type.
        """
        return self._value

    ### Overidden from ast.Node.
    
    def validate(self, vld):
        self._type.validate(vld)
        self._type.validateCtor(vld, self._value)
    
    def pac(self, printer):
        self._type.pacCtor(printer, self._value)

    ### Overidden from Expression.

    def type(self):
        return self._type
    
    def evaluate(self, cg):
        return self._type.hiltiCtor(cg, self._value)
    
    def __str__(self):
        return str(self._value)
    
class Name(Assignable):
    """An expression referencing an identifier.
    
    name: string - The name of the ID.
    scope: ~~Scope - The scope in which to evaluate the ID.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, name, scope, location=None):
        super(Name, self).__init__(location=location)
        self._name = name
        self._scope = scope

    def name(self):
        """Returns the name that is referenced."""
        return self._name
        
    def _internalName(self):
        """Maps user-visible name to internal name.
        
        Todo:  Not sure this is the best place for this ...
        """
        if self._name == "self":
            return "__self"
        
        return self._name
        
    ### Overidden from ast.Node.
    
    def validate(self, vld):
        if not self._scope.lookupID(self._name):
            vld.error(self, "unknown identifier %s" % self._name)
        
    def pac(self, printer):
        printer.output(self._name)

    def simplify(self):
        expr = super(Name, self).simplify()
        if expr:
            return expr

        i = self._scope.lookupID(self._name)
        assert i
        
        if isinstance(i, id.Constant):
            return Constant(i.value())
    
        return self
        
    ### Overidden from Expression.

    def type(self):
        id = self._scope.lookupID(self._name)
        return id.type() if id else type.Unknown(self._name, location=self.location())

    def evaluate(self, cg):
        name = self._internalName()
        return cg.functionBuilder().idOp(name)
    
    def __str__(self):
        return self._name

    ### Overidden from Assignable.

    def assign(self, cg, rhs):
        i = self._scope.lookupID(self._name)
        assert i
        
        if isinstance(i, id.Global) or isinstance(i, id.Local) or isinstance(i, id.Parameter):
            name = self._internalName()
            cg.builder().assign(cg.functionBuilder().idOp(name), rhs)
            
        else:
            util.internal_error("unexpected id type %s in NameExpr::assign", repr(i))
            
        
class Assign(Expression):
    """An expression assigning a value to a destination.
    
    dest: ~~Expression - The destination expression.
    rhs: ~~Expression - The value to assign.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, dest, rhs, location=None):
        super(Assign, self).__init__(location=location)
        self._dest = dest
        self._rhs = rhs

    ### Overidden from ast.Node.
    
    def validate(self, vld):
        self._dest.validate(vld)
        self._rhs.validate(vld)
        
        if "assign" in self._dest.__dict__:
            vld.error(self, "cannot assign to lhs expression")
        
        if self._dest.type() != self._rhs.type():
            vld.error(self, "types do not match in assigment")

        if self._dest.isConst():
            vld.error(self, "cannot assign to constant")
            
    def pac(self, printer):
        self._dest.pac(printer)
        printer.output(" = ")
        self._rhs.pac(printer)

    ### Overidden from Expression.

    def type(self):
        return self._dest.type()
    
    def evaluate(self, cg):
        rhs = self._rhs.evaluate(cg)
        self._dest.assign(cg, rhs)
    
    def __str__(self):
        return self._name
    
