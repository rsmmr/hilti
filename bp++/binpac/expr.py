#! $Id$ 
#
# BinPAC++ expressions.

import type
import node
import grammar
import operator
import id
import binpac.util as util

import hilti.operand

class Expression(node.Node):
    """Base class for all expression objects.

    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Expression, self).__init__(location)

    def simplify(self):
        """Simplifies the expression. The definition of 'simplify' is left to
        the derived classes but the expression may only change in a way thay
        does not modify its semantics.
        
        Can be overridden by derived classes. The default implementation
        returns just *self*.
        
        Returns: ~~Expression - The simplified expression, which may be just *self*.
        """
        return self
        
    def canCoerceTo(self, dsttype):
        """Returns whether the expression can be coerce to a given target type.
        If *dsttype* is of the same type as that of the expression, the result
        is always True. 
        
        *dstype*: ~~Type - The target type.
        
        Returns: bool - True if the expression can be coerceed. 
        """
        if self.isInit():
            # We just try in the case. 
            try:
                operator.coerceCtorTo(self, dsttype)
                return True # It worked.
            except operator.CoerceError:
                return False # It did not.
            
        else:
            return operator.canCoerceExprTo(self, dsttype)
        
    def coerceTo(self, dsttype, cg=None):
        """Coerces the expression to a differenent type. It's ok if *dsttype* is
        the same as the expressions type, the method will be a no-op then. 
        
        *dsttype*: ~~Type - The type to coerce the expression into. 
        
        *cg*: ~~CodeGen - The current code generator; can be None if the
        expression is a ~~Ctor. 
        
        Returns: ~~Expression - The resulting  expression of the target type
        with the coerceed expression.
        
        Throws: operator.CoerceError - If the conversion is not possible.  This
        will only happen if ~~canCoerceTo returns *False*.
        """
        if not self.canCoerceTo(dsttype):
            raise operator.CoerceError, "cannot convert type %s to type %s" % (self.type(), dsttype)

        if isinstance(self, Ctor):
            try:
                return operator.coerceCtorTo(self, dsttype)
            except operator.CoerceError:
                # Can't happen because canCoerceTo() returned true ...
                util.internal_error("unexpected coerce error from %s to %s" % (self.type(), dsttype()))
            
        else:
            assert cg 
            return operator.coerceExprTo(cg, self, dsttype)
    
    def isInit(self):
        """Returns true if the expression evaluates to a value that can be
        used as the initialization value for a HILTI object. 

        Returns: bool - Boolean indicating whether the expression is an
        initialization value."""
        e = self.simplify()
        
        return isinstance(e, Ctor)
    
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

    ### Overidden from node.Node.

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        for expr in self._exprs:
            if isinstance(expr, Expression):
                expr.resolve(resolver)

        #if operator.typecheck(self._op, self._exprs):
        #    operator.resolve(self._op, resolver, self._exprs)
            
    def validate(self, vld):
        Expression.validate(self, vld)
        for expr in self._exprs:
            if isinstance(expr, Expression):
                expr.validate(vld)
            
        if not operator.typecheck(self._op, self._exprs):
            types = ", ".join([str(e.type()) for e in self._exprs])
            vld.error(self, "no match for overloaded operator %s with types (%s)" % (self._op, types))
            
        operator.validate(self._op, vld, self._exprs)

    def pac(self, printer):
        operator.pacOperator(printer, self._op, self._exprs)

    ### Overidden from Expression.

    def type(self):
        t = operator.type(self._op, self._exprs)
        assert t
        return t

    def simplify(self):
        self._exprs = [(e.simplify() if isinstance(e, node.Node) else e) for e in self._exprs]
        expr = operator.simplify(self._op, self._exprs)
        return expr if expr else self
    
    def evaluate(self, cg):
        return operator.evaluate(self._op, cg, self._exprs)

    def assign(self, cg, rhs):
        return operator.assign(self._op, cg, self._exprs, rhs)
    
    def __str__(self):
        return "(%s %s)" % (self._op, " ".join([str(e) for e in self._exprs]))
    
class Not(Expression):
    """Class for expressions inverting a boolean value. 

    expr: ~~Expression - The operand expression.
    
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, expr, location=None):
        super(Not, self).__init__(location=location)
        self._expr = expr

    def expr(self):
        """Returns the expression's operand.
        
        Returns: ~~Expression - The operand.
        """
        return self._expr

    ### Overidden from node.Node.

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        self._expr.resolve(resolver)
            
    def validate(self, vld):
        Expression.validate(self, vld)
        self._expr.validate(vld)
        
        if not self._expr.canCoerceTo(type.Bool()):
            vld.error("expression must be a boolean")

    def pac(self, printer):
        pass

    ### Overidden from Expression.

    def type(self):
        return type.Bool()

    def simplify(self):
        self._expr = self._expr.simplify()
        return self
        
    def evaluate(self, cg):
        e = self._expr.coerceTo(type.Bool(), cg).evaluate(cg)
        b = cg.builder().addLocal("__bool", hilti.type.Bool())
        cg.builder().select(b, e, cg.builder().constOp(False), cg.builder().constOp(True))
        return b
    
    def __str__(self):
        return "(not %s)" % self._expr

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
        
        assert not isinstance(value, Expression)
        
    def value(self):
        """Returns the value of the ctor expression.
        
        Returns: any - The value of the ctor of a type-specific type.
        """
        return self._value

    ### Overidden from node.Node.

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        self._type = self._type.resolve(resolver)
    
    def validate(self, vld):
        Expression.validate(self, vld)
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
        self._is_init = False

    def name(self):
        """Returns the name that is referenced."""
        return self._name
        
    ### Overidden from node.Node.

    def resolve(self, resolver):
        Assignable.resolve(self, resolver)
        
        i = self._scope.lookupID(self._name)
        if not i:
            # Will be reported in validate.
            return

        self._is_init = isinstance(i, id.Constant)
        
    def validate(self, vld):
        Assignable.validate(self, vld)
        if not self._scope.lookupID(self._name):
            vld.error(self, "unknown identifier %s" % self._name)
        
    def pac(self, printer):
        printer.output(self._name)

    ### Overidden from Expression.

    def type(self):
        id = self._scope.lookupID(self._name)
        return id.type() if id else type.Unknown(self._name, location=self.location())

    def evaluate(self, cg):
        assert self._scope
        i = self._scope.lookupID(self._name)
        assert i
        return i.evaluate(cg)

    def isInit(self):
        return self._is_init
    
    def __str__(self):
        return self._name

    ### Overidden from Assignable.

    def assign(self, cg, rhs):
        assert self._scope
        i = self._scope.lookupID(self._name)
        assert i
        cg.builder().assign(i.evaluate(cg), rhs)
        
        
class Attribute(Expression):
    """An expression referencing an attribute of another object. This
    expression has always type ~~String.
    
    name: string - The name of the attribute.
    
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, name, location=None):
        super(Attribute, self).__init__(location=location)
        self._name = name

    def name(self):
        """Returns the attribute name that is referenced."""
        return self._name
        
    ### Overidden from node.Node.
    
    def pac(self, printer):
        printer.output(self._name)

    ### Overidden from Expression.

    def type(self):
        return type.String()

    def __str__(self):
        return self._name
    
    def __eq__(self, other):
        return self._name == other._name
            
class Type(Expression):
    """An expression representing a BinPAC++ type.
    
    ty: ~~Type - The type represented by the expression.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, ty, location=None):
        super(Expression, self).__init__(location=location)
        self._type = ty

    def referencedType(self):
        """Returns the referenced type.
        
        Returns: ~~Type - The type.
        """
        return self._type
        
    ### Overidden from node.Node.

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        self._type = self._type.resolve(resolver)
    
    def validate(self, vld):
        Expression.validate(self, vld)
        self._type.validate(vld)
        
    def pac(self, printer):
        self._type.pac(printer)

    ### Overidden from Expression.

    def type(self):
        return type.MetaType(self._type, location=self.location())
    
    def __str__(self):
        return str(self._type)

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

    ### Overidden from node.Node.

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        
        self._dest.resolve(resolver)
        self._rhs.resolve(resolver)
    
    def validate(self, vld):
        Expression.validate(self, vld)
        
        self._dest.validate(vld)
        self._rhs.validate(vld)
        
        if "assign" in self._dest.__dict__:
            vld.error(self, "cannot assign to lhs expression")
        
        if self._dest.type() != self._rhs.type():
            vld.error(self, "types do not match in assigment")

        if self._dest.isInit():
            vld.error(self, "cannot assign to an init expression")
            
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
        return str(self._dest) + " = " + str(self._rhs)
    
class Hilti(Expression):
    """An expression encapsulating an already eveluated HILTI operand. This
    enables HILTI operands to be passed into evaluation functions that expect
    expressions.
    
    op: ~~hilti.operand.Operand - The HILTI operand.
    
    ty: ~~Type - The BinPAC++ type that the operand has. 

    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, op, ty, location=None):
        super(Hilti, self).__init__(location)
        self._op = op
        self._type = ty

    ### Methods for derived classes to override.    
    
    def type(self):
        return self._type

    def resolve(self, resolver):
        Expression.resolve(self, resolver)
        self._type = self._type.resolve(resolver)

    def validate(self, vld):
        Expression.validate(self, vld)
        self._type.validate(vld)
        
    def evaluate(self, cg):
        return self._op

    def __str__(self):
        return "<HILTI operand>"
