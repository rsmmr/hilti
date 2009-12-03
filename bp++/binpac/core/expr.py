#! $Id$ 
#
# BinPAC++ expressions.

import type
import ast
import grammar
import operator
import binpac.support.util as util

import hilti.core.instruction

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

    def canCastTo(self, dsttype):
        """Returns whether the expression can be cast to a given target type.
        If *dsttype* is of the same type as that of the expression, the result
        is always True. 
        
        *dstype*: ~~Type - The target type.
        
        Returns: bool - True if the expression can be casted. 
        """
        
        if self.isConst():
            const = self.fold()
            assert isinstance(const, Constant)
            
            # We just try the case. 
            try:
                operator.castConstTo(const, dsttype)
                return True # It worked.
            except operator.CastError:
                return False # It did not.
            
        else:
            return operator.canCastNonConstExprTo(self, dsttype)
        
    def castTo(self, dsttype, codegen=None, builder=None):
        """Casts the expression to a differenent type. It's ok if *dsttype* is
        the same as the expressions type, the method will be a no-op then. 
        
        *dsttype*: ~~Type - The type to cast the expression into. 
        
        *codegen*: ~~CodeGen - The current code generator; can be None if the
        expression is constant. 
        
        *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use;
        can be None if the expression is constant. 
        
        Returns: tuple (~~Expression, ~~hilti.core.builder.Builder) - The
        first element is a new expression of the target type with the casted
        expression; the second element is the builder to use for subsequent
        code.
        
        Throws: operator.CastError - If the conversion is not possible.  This
        will only happen if ~~canCastTo returns *False*.
        """
        if not self.canCastTo(dsttype):
            raise operator.CastError, "cannot convert type %s to type %s" % (self.type(), dsttype)

        if self.isConst():
            const = self.fold()
            assert isinstance(const, Constant)
            try:
                const = operator.castConstTo(const, dsttype)
                return (const, builder)
            except operator.CastError:
                # Can't happen because canCastTo() returned true ...
                util.internal_error("unexpected cast error from %s to %s" % (self.type(), dsttype()))
            
        else:
            assert builder and codegen 
            return operator.castNonConstExprTo(codegen, builder, self, dsttype)
    
    def isConst(self):
        """Returns true if the expression evaluates to a constant value.
        
        Must be overidden by derived classes.
        
        Returns: bool - Boolean indicating whether the expression is a constant.
        """
        if isinstance(self, Constant):
            return True
        
        # Test if we can fold us into a constant.
        if isinstance(self.fold(), Constant):
            return True
        
        return False

    ### Methods for derived classes to override.    
    
    def fold(self):
        """Performs constant folding. 
        
        Can be overidden by derived classes.
        
        Returns: ~~expression.Constant - The folded expression if foldable, or
        None if not (including for non-constant expression). 
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

    def evaluate(self, codegen, builder):
        """Generates code to evaluate the expression.
 
        Must be overidden by derived classes. 
 
        *codegen*: ~~CodeGen - The current code generator.
        *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
        
        Returns: tuple (~~hilti.core.instruction.Operand,
        ~~hilti.core.builder.Builder) - The first element is an operand with
        the value of the evaluated expression; the second element is the
        builder to use for subsequent code. 
        """
        util.internal_error("Expression::evaluate not overridden in %s", self.__class__)
    
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
        
    ### Methods for derived classes to override.    

    def _subEvaluate(self, codegen, builder, expr):
        """Evaluates the unary expression given an already evaluated operand.
        
        Must be overidden by derived classes. 
        
        *codegen*: ~~CodeGen - The current code generator.
        *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
        *expr*: ~~hilti.core.instruction.Operand - The already evaluated
        operand of the unary expression.
        
        Returns: tuple (~~hilti.core.instructin.Operand,
        ~~hilti.core.builder.Builder) - The first element is an operand with
        the value of the evaluated expression; the second element is the
        builder to use for subsequent code.         """
        util.internal_error("expression.Binary._subEvaluate not overidden for %s" % self.__class__)
        
    ### Overidden from Expression.
        
    def evaluate(self, codegen, builder):
        (expr, builder) = self._expr.evaluate(codegen, builder)
        return self._subEvaluate(codegen, builder, expr)
    
    def __str__(self):
        return "<unary-expression>"
    
class Binary(Expression):
    """Base class for expressions depending on two other expressions.

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

    ### Methods for derived classes to override.    

    def _subEvaluate(self, codegen, builder, expr1, expr2):
        """Evaluates the binary expression given already evaluated operands.
        
        Must be overidden by derived classes. 
        
        *codegen*: ~~CodeGen - The current code generator.
        *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
        *expr1*: ~~hilti.core.instruction.Operand - The already evaluated
        operand 1 of the binary expression.
        *expr2*: ~~hilti.core.instruction.Operand - The already evaluated
        operand 2 of the binary expression.
        
        Returns: tuple (~~hilti.core.instructin.Operand,
        ~~hilti.core.builder.Builder) - The first element is an operand with
        the value of the evaluated expression; the second element is the
        builder to use for subsequent code. 
        """
        util.internal_error("expression.Binary._subEvaluate not overidden for %s" % self.__class__)
    
    ### Overidden from Expression.

    def evaluate(self, codegen, builder):
        (expr, builder) = self._expr.evaluate(codegen, builder)
        return self._subEvaluate(codegen, builder, self._exprs[0], self._exprs[1])
    
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

    ### Overidden from Expression.
        
    def fold(self):
        return operators.fold(self._op, self.expr())

    def __str__(self):
        return "(%s %s)" % (self._op, self.expr())

    ### Overidden from UnaryExpression.
    
    def _subEvaluate(self, codegen, builder, expr):
        return operators.evaluate(self._op, codegen, builder, self.expr())
    
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

    ### Overidden from Expression.
        
    def fold(self):
        e = self.exprs()
        return operators.fold(self._op, e[0], e[1])

    def __str__(self):
        e = self.exprs()
        return "(%s %s %s)" % (self._op, e[0], e[1])

    ### Overidden from BinaryExpression.
    
    def _subEvaluate(self, codegen, builder, expr1, expr2):
        return operators.evaluate(self._op, codegen, builder, expr1, expr2)
    
class Constant(Expression):
    """A constant expression.
    
    const: ~~Constant - The constant.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, const, location=None):
        super(Constant, self).__init__(const.type(), location=location)
        self._const = const
        
    def constant(self):
        """Returns the constant value.
        
        Returns: ~~Constant - The constant.
        """
        return self._const
    
    ### Overidden from Expression.

    def evaluate(self, codegen, builder):
        c = self._const.type().hiltiConstant(self._const, codegen, builder)
        return (hilti.core.instruction.ConstOperand(c), builder)
    
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

    ### Overidden from Expression.
        
    def fold(self):
        return None

    def evaluate(self, codegen, builder):
        util.internal_error("expr.ID.evaluate not implemented")
    
    def __str__(self):
        return self._id.name()

class HILTIOperand(Expression):
    """An expression refering an already built HILTI Operand.
    
    op: ~~hilti.core.instruction.Operand - The HILTI operand. 
    ty: ~~Type - The BinPAC type *op*'s type corresponds to.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, op, ty, location=None):
        super(HILTIOperand, self).__init__(ty, location=location)
        self._op = op
        
    def fold(self):
        return None

    def evaluate(self, codegen, builder):
        return (self._op, builder)
    
    def __str__(self):
        return self._id.name()
        
