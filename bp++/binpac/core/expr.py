#! $Id$ 
#
# BinPAC++ expressions.

import type
import ast
import grammar
import operator
import binpac.support.util as util

class Expression(ast.Node):
    """Base class for all expression objects.

    All derived classes must throw ValueError in their constructor if they
    encounter invalid arguments.
    
    type: ~~Type - The type the expression evaluates to.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, type, location=None):
        super(Expression, self).__init__(location)
        self._type = type
        
    def type(self):
        """Returns the type to which the expression evaluates to.
        
        Returns: ~~Type - The type.
        """
        return self._type
        
    def isConst(self):
        """Returns true if the expression evaluates to a constant value.
        
        Must be overidden by derived classes.
        
        Returns: bool - Boolean indicating whether the expression is a constant.
        """
        util.internal_error("Expression::isConst not overridden in %s", self.__class__)

    def fold(self):
        """Performs constant folding. This must only be
        called for expressions for which ~~isConst returns True. 

        Can be overidden by derived classes.
        
        Returns: ~~Expression - The folded expression if foldable, or `self`
        if not.
        """
        return self
        
    def simplify(self):
        """Simplifies the expression. A expression is only modified if
        possible in a way that does not change its value, and potentially not
        even then. 
        
        The base class implements constant folding.
        
        Can be overidden by derived classes, in which case the superclass'
        version must be called first. 
        
        Returns: ~~Expression - The simplified expression (which may be just
        +self+).
        """
        
        expr = self
        
        # Constant folding.
        if self.isConst():
            expr = expr.fold()
        
        return expr
        
    def __str__(self):
        return "<generic-expression>"
        
class Unary(Expression):
    """Base class for expressions depending on a single other expression.

    Implements ~~isConst for the derived classes.
    
    expr: ~~Expression - The expression this one depends on.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, expr, location=None):
        super(Unary, self).__init__(expr.type(), location=location)
        self._expr = expr
        
    def expr(self):
        """Returns the other expression this expression depends on.
        
        Returns: ~~Expression - The other expression.
        """
        return self._expr

    def isConst(self):
        return self._expr.isConst()

    def __str__(self):
        return "<unary-expression>"
    
class Binary(Expression):
    """Base class for expressions depending on two other expressions.

    Implements ~~isConst for the derived classes.
    
    exprs: tuple (~~Expression, ~~Expression) - The expressions this one depends on.
    type: ~~Type - The type the expression evaluates to.
    
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, exprs, t, location=None):
        super(Binary, self).__init__(t, location=location)
        self._exprs = exprs
        
    def exprs(self):
        """Returns the other expressions this expression depends on.
        
        Returns: tuple (~~Expression, ~~Expression) - The other expressions.
        """
        return self._exprs

    def isConst(self):
        return self._exprs[0].isConst() and self._exprs[1].isConst()

    def __str__(self):
        return "<binary-expression>"

class OverloadedUnary(Unary):
    """A unary expression for an operator overloaded by the types of its
    operand.
    
    op: string - The name of the operator.
    expr: ~~Expression - The expression this one depends on.
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, op, expr, location=None):
        if not operators.typecheck(op, expr):
            util.error("no match for overloaded operator", context=location)
        
        t = operators.type(op, expr)
        assert t
        
        super(OverloadedUnary, self).__init__(expr, t, location=location)
        self._op = op
        
    def fold(self):
        return operators.fold(self._op, self.expr())

    def __str__(self):
        return "(%s %s)" % (self._op, self.expr())
    
class OverloadedBinary(Binary):
    """A binary expression for an operator overloaded by the types of its
    operands.
    
    op: ~~Operator - The operator.
    exprs: tuple (~~Expression, ~~Expression) - The expressions this expression depends
    on, with their number determined by the operator.
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, op, exprs, location=None):
        if not operators.typecheck(op, *exprs):
            util.error("no match for overloaded operator", context=location)
        
        t = operators.type(op, *exprs)
        assert t
        
        super(OverloadedBinary, self).__init__(exprs, t, location=location)
        self._op = op
        
    def fold(self):
        e = self.exprs()
        return operators.fold(self._op, e[0], e[1])

    def __str__(self):
        e = self.exprs()
        return "(%s %s %s)" % (self._op, e[0], e[1])
        
class Constant(Expression):
    """A constant expression.
    
    const: ~~Constant - The constant.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, const, location=None):
        super(Constant, self).__init__(const.type(), location=location)
        self._const = const
        
    def isConst(self):
        return True

    def constant(self):
        """Returns the constant value.
        
        Returns: ~~Constant - The constant.
        """
        return self._const
    
    def __str__(self):
        return str(self._const.value())
    
class ID(Expression):
    """An expression referencing an ID.
    
    id: ~~id.ID - The ID.
    scope: ~~Scope - The scope in which to evaluate the ID.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, id, scope, location=None):
        assert scope.lookup(id.name())
        super(Constant, self).__init__(id.type(), location=location)
        self._id = id
        self._scope = scope
        
    def isConst(self):
        return False
    
    def fold(self):
        raise ValueError
    
    def __str__(self):
        return self._id.name()
    
    
