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
        
    def castTo(self, dsttype, cg=None):
        """Casts the expression to a differenent type. It's ok if *dsttype* is
        the same as the expressions type, the method will be a no-op then. 
        
        *dsttype*: ~~Type - The type to cast the expression into. 
        
        *cg*: ~~CodeGen - The current code generator; can be None if the
        expression is constant. 
        
        Returns: Expression - The new expression of the target type
        with the casted expression.
        
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
        if isinstance(self.fold(), Constant):
            return True
        
        return False

    ### Methods for derived classes to override.    

    def type(self):
        """Returns the type to which the expression evaluates to.

        Must be overidden by derived classes.
        
        Returns: ~~Type - The type.
        """
        util.internal_error("Expression::type not overridden in %s", self.__class__)
    
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

    def evaluate(self, cg):
        """Generates code to evaluate the expression.
 
        Must be overidden by derived classes. 
 
        *cg*: ~~CodeGen - The current code generator.
        
        Returns: ~~hilti.core.instruction.Operand - An operand with
        the value of the evaluated expression.
        """
        util.internal_error("Expression::evaluate not overridden in %s", self.__class__)

    def __str__(self):
        return "<generic expression>"
        
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

    def validate(self, vld):
        for expr in self._exprs:
            if isinstance(expr, Expression):
                expr.validate(vld)
            
        if not operator.typecheck(self._op, self._exprs):
            vld.error(self, "no match for overloaded operator")
            
        operator.validate(self._op, vld, self._exprs)

    def pac(self, printer):
        operator.pacOperator(printer, self._op, self._exprs)
        
    ### Overidden from Expression.

    def type(self):
        t = operator.type(self._op, self._exprs)
        assert t
        return t

    def fold(self):
        return operator.fold(self._op, self._exprs)

    def evaluate(self, cg):
        return operator.evaluate(self._op, cg, self._exprs)

    def __str__(self):
        return "(%s %s)" % (self._op, " ".join([str(e) for e in self._exprs]))
    
class Constant(Expression):
    """A constant expression.
    
    const: ~~Constant - The constant.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, const, location=None):
        super(Constant, self).__init__(location=location)
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
        return hilti.core.instruction.ConstOperand(c)
    
    def __str__(self):
        return str(self._const.value())
    
class Name(Expression):
    """An expression referencing an identifier.
    
    name: string - The name of the ID.
    scope: ~~Scope - The scope in which to evaluate the ID.
    location: ~~Location - The location where the expression was defined. 
    """
    
    def __init__(self, name, scope, location=None):
        super(Name, self).__init__(location=location)
        self._name = name
        self._scope = scope

    ### Overidden from ast.Node.
    
    def validate(self, vld):
        if not self._scope.lookupID(self._name):
            vld.error(self, "unknown identifier %s" % self._name)
        
    def pac(self, printer):
        printer.output(self._name)
        
    ### Overidden from Expression.
    
    def type(self):
        id = self._scope.lookupID(self._name)
        return id.type() if id else type.Unknown(self._name, location=self.location())
    
    def fold(self):
        return None

    def evaluate(self, cg):
        return cg.functionBuilder().idOp(self._name)
    
    def __str__(self):
        return self._name
