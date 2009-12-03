# $Id$
"""
Overloading operators for BinPAC++ types. 
-----------------------------------------

To overload an operator, you define a new class with methods to implement the
functionality. The class is decorated with the operator it is overloading and
the expression types it applies to. 

For example, the following class overloads the plus-operator for expressions
of integer type::

    from operators import *
    from core.type import integer

    @operator.plus(type.Integer, type.Integer)
    class Plus:
        def type(expr1, expr2):
            # Result type of expression.
            return type.Integer()
    
        def codegen(expr1, expr2):
            [... generate HILTI code ...]
            

Once defined, the BinPAC++ compiler will call `codegen` to generate HILTI code
for all sums integer expressions. Likewise, it will call ``type` when it needs
to know the result type of evaluating the operators. While these two methods
are mandatory, there are further optional methods can be added to the class as
needed; see below. 

The definition of the class must be located inside a submodule of the BinPAC++
`pactypes` package, and it's recommended to group the classes by the types
they support (e.g., putting all integer operators intp a subpackage
`packtypes.integer`.).

Class decorators
================

There is one decorator for each operator provided by the BinPAC++ language.
The arguments for decorator are instances of ~~type.Type, corresponding to the
types of expressions the overloaded operator applies to. The number of types
is specific to each operator.  The matching of expression type to operators is
performed via Python's `isinstance` and thus inheritance relatioships might be
exploited by specifiying a base class in the decorator's arguments. 

The following operators are available:

XXX include auto-generated text

Class methods for operators
===========================

A class decorated with @operator.* may provide the following methods. Most of
them receive instances of ~~Expression objects as arguments, with the number
determined by the operator type.  Note these methods do not receive a *self*
attribute; they are all class methods.

.. function:: typecheck(<exprs>)

  Optional. Adds additional type-checking supplementing the simple
  +isinstance+ test performed by default. If implemented, it is guarenteed
  that all other class methods will only be called if this check returns True.
  
  Returns: bool - True if the operator applies to these expression types.

.. function:: fold(<exprs>)

  Optional. Implement constant folding.
  
  Returns: ~~expr.Constant - A new expression object representing the folded
  expression. 

.. function:: type(<exprs>)

  Mandatory. Returns the result type this operator evaluates the operands to.

  Returns: ~~type.Type - An type *instance* defining the result type.

.. function:: evaluate(codegen, builder, <exprs>)

  Mandatory. Implements evaluation of the operator, generating the
  corresponding HILTI code. 

  *codegen*: ~~CodeGen - The current code generator.
  *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.

  Returns: tuple (~~hilti.core.instructin.Operand,
  ~~hilti.core.builder.Builder) - The first element is an operand with
  the value of the evaluated expression; the second element is the
  builder to use for subsequent code. 

.. function:: castConstTo(const, dsttype):

   Optional. Casts a constant of the operator's type to another type. This may or
   may not be possible; if not, the method must throw a ~~CastError.
        
   *const*: ~~Constant - The constant to cast. 
   *dsttype*: ~~Type - The type to cast the constant into.
        
   Returns: ~~Constant - A constant of *dsttype* with the casted value.
        
   Throws: ~~CastError if the cast is not possible. There's no need to augment
   the exception with an explaining string as that will be added automatically.

.. function:: canCastNonConstExprTo(expr, dsttype):

   Optional. Checks whether we can cast a non-constant expression of the
   operator's type to another type. 
   
   *expr*: ~~Expr - The expression to cast. 
   *dsttype*: ~~Type - The type to cast the expression into.

   Note: The return value of this method should match with what ~~castConstTo
   and ~~castExprTo are able to do. If in doubt, this function should accept a
   superset of what those methods can do (with them then raising exceptions if
   necessary). 
   
   Returns: bool - True if the cast is possible. 
   
.. function:: castNonConstExprTo(codegen, builder, expr, dsttype):
   
   Optional. Casts a non-constant expression of the operator's type to another
   type. This function will only be called if the corresponding
   ~~canCastNonConstExprTo indicates that the cast is possible. 
        
   *codegen*: ~~CodeGen - The current code generator.
   *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
   *expr*: ~~Expression - The expression to cast. 
   *dsttype*: ~~Type - The type to cast the expression into.
        
   Returns: tuple (~~Expression, ~~hilti.core.builder.Builder) - The first
   element is a new expression of the target type with the casted expression;
   the second element is the builder to use for subsequent code.
"""

import inspect
import functools

import binpac.support.util as util

### Available operators and operator methods.

_Operators = [
    # Operator name, #operands, description.
    ("Plus", 2, "The sum of two expressions. (`a + b`)"),
    ("Minus", 2, "The difference of two expressions. (`a + b`)"),
    ("Mult", 2, "The product of two expressions. (`a * b`)"),
    ("Div", 2, "The division of two expressions. (`a / b`)"),
    ("Neg", 1, "The negation of an expressions. (`- a`)"),
    ("Cast", 1, "Cast into another type."),
    ]

_Methods = ["typecheck", "fold", "codegen", "type", "castConstTo", "canCastNonConstExprTo", "castNonConstExprTo"]
    
### Public functions.    
    
def typecheck(op, *args):
    """Checks whether an operator is compatible with a set of operand
    expressions. 
    
    op: ~~Operator - The operator.
    args: one or more ~~Expressions - The expressions for the operator.
    
    Returns: bool - True if operator and expressions are compatible.
    """
    func = _findOp("typecheck", op, args)
    if not func:
        # Not matching operator found.
        return False
    
    return func(*args)
    
def fold(op, *args):
    """Evaluates an operator with constant expressions.
    
    op: ~~Operator - The operator.
    args: one or more ~~Expressions - The expressions for the operator.
    
    Returns: ~~expr.Constant - An new expression, representing the folded
    expression; or None if the operator does not support constant folding.
    
    Note: This function must only be called if ~~typecheck indicates that
    operator and expressions are compatible. 
    """
    if not typecheck(op, *args):
        util.error("no fold implementation for %s operator with %s" % (op, _fmtArgTypes(args)))

    func = _findOp("fold", op, args)
    result = func(*args) if func else None
    assert not result or isinstance(result, expression.Constant) 
    return result

def evaluate(op, codegen, builder, *args):
    """Generates HILTI code for an expression.
    
    This function must only be called if ~~typecheck indicates that operator
    and expressions are compatible. 

    op: string - The name of the operator.
    *codegen*: ~~CodeGen - The current code generator.
    *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
    
    args: one or more ~~Expressions - The expressions for the operator.

    Returns: tuple (~~hilti.core.instructin.Operand,
    ~~hilti.core.builder.Builder) - The first element is an operand with
    the value of the evaluated expression; the second element is the
    builder to use for subsequent code. 
    """
    if not typecheck(op, *args):
        util.error("no matching %s operator for %s" % (op, _fmtArgTypes(args)))

    func = _findOp("evaluate", op, args)
    
    if not func:
        util.error("no evaluate implementation for %s operator with %s" % (op, _fmtArgTypes(args)))
    
    return func(codegen, builder, *args)

def type(op, *args):
    """Returns the result type for an operator.
    
    This function must only be called if ~~typecheck indicates that operator
    and expressions are compatible.

    op: string - The name of the operator.
    args: one or more ~~Expressions - The expressions for the operator.
    
    Returns: ~~type.Type - The result type.
    """
    if not typecheck(op, *args):
        util.error("no matching %s operator for %s" % (op, _fmtArgTypes(args)))

    func = _findOp("type", op, args)
    
    if not func:
        util.error("no type implementation for %s operator with %s" % (op, _fmtArgTypes(args)))
    
    return func(*args)

class CastError(Exception):
    pass

def canCastNonConstExprTo(expr, dsttype):
    """Returns whether an non-constant expression can be cast to a given
    target type. If *dsttype* is of the same type as the expression, the
    result is always True. 
    
    *expr*: ~~Expression - The expression to check. 
    *dstype*: ~~Type - The target type.
        
    Returns: bool - True if the expression can be casted. 
    """
    ty = expr.type()
    
    if ty == dsttype:
        return True

    if not typecheck(Cast, ty):
        return False
    
    func = _findOp(Cast, "canCastNonConstExprTo", ty)
    
    if not func:
        return False
    
    return func(expr, dsttype)
    
def castNonConstExprTo(codegen, builder, expr, dsttype): 
    """
    Casts a non-constant expression of one type into another. This operator must only
    be called if ~~canCastNonConstExprTo indicates that the cast is supported. 
    
    *codegen*: ~~CodeGen - The current code generator.
    *builder*: ~~hilti.core.builder.Builder - The HILTI builder to use.
    expr: ~~expr - The expression to cast. 
    dsttype: ~~Type - The type to cast the expression into. 
    
    Returns: tuple (~~Expression, ~~hilti.core.builder.Builder) - The first
    element is a new expression of the target type with the casted expression;
    the second element is the builder to use for subsequent code.
    """

    assert canCastNonConstExpr(expr, dsttype)

    ty = expr.type()
    
    if ty == dsttype:
        return (expr, builder)

    if not typecheck(Cast, ty):
        return False
    
    func = _findOp(Cast, "castNonConstExprTo", ty)
    
    if not func:
        return False
    
    return func(codegen, builder, expr, dsttype)
    

def castConstTo(const, dsttype): 
    """
    Casts a constant of one type into another. This may or may not be
    possible; if not, a ~~CastError is thrown.

    const: ~~Constant - The constant to cast. 
    dsttype: ~~Type - The type to cast the constant into. 
    
    Returns: ~~Constant - A new constant of the target type with the casted
    value.
        
    Throws: ~~CastError if the cast is not possible.
    """
    ty = const.type()

    if ty == dsttype:
        return const
    
    if not typecheck(Cast, ty):
        raise CastError
    
    func = _findOp(Cast, "castConstantTo", ty)
    
    if not func:
        raise CastError
    
    return func(const, dsttype)
    
class Operator:
    """Constants defining the available operators."""
    # Note: we add the constants dynamically to this class' namespace, see
    # below. 
    pass

def generateDecoratorDoc(file):
    """Generates documentation for the available operator decorators. The
    documentation will be included into the developer manual.
    
    file: file - The filo into which the output will be written.
    """
    for (op, args, descr) in _Operators.sorted():
        args = ", ".join(["type"] * args)
        print >>file, ".. function @operator.%s(%s)" % (op, args)
        print >>file
        print >>file, "   %s" % descr
        print >>file
            
### Internal code for keeping track of registered operators.

_OverloadTable = {}

def _registerOperator(operator, f, args):
    try:
        _OverloadTable[operator] += [(f, args)]
    except KeyError:
        _OverloadTable[operator] = [(f, args)]

def _default_typecheck(*args):
    return True

def _fmtTypes(types):
    return ",".join([str(t) for t in types])

def _fmtArgTypes(args):
    return " ".join([str(a.type()) for a in args])

def _makeOp(op, *args):
    def __makeOp(cls):
        
        for m in _Methods:
            if m in cls.__dict__:
                f = cls.__dict__[m]
                if len(inspect.getargspec(f)[0]) != len(args):
                    util.internal_error("%s has wrong number of argument for %s" % (m, _fmtTypes(args)))
                
                _registerOperator((op, m), f, args)

        if not "typecheck" in cls.__dict__:
            # Add a "always true" typecheck if there's no other defined.
            _registerOperator((op, "typecheck"), _default_typecheck, args)
                
        return cls
    
    return __makeOp
    
class _Decorators:    
    def __init__(self):
        for (op, args, descr) in _Operators:
            globals()[op] = functools.partial(_makeOp, op)

def _findOp(method, op, args):
    assert method in _Methods
    
    try:
        ops = _OverloadTable[(op, method)]
    except KeyError:
        return None

    matches = []
    
    for (func, types) in ops:
        if len(types) != len(args):
            continue
        
        found = True
        for (a, t) in zip(args, types):
            if not isinstance(a.type(), t):
                found = False

        if found:
            matches += [(func, types)]

    if not matches:
        return None
            
    if len(matches) == 1:
        return matches[0][0]

    msg = "ambigious operator definitions: types %s match\n" % _fmtArgTypes(args)
    for (func, types) in matches:
        msg += "    %s\n" % _fmtTypes(types)
        
    util.internal_error(msg)

### Initialization code.

# Create "operator" namespace.
operator = _Decorators()            

# Add operator constants to class Operators.
for (op, args, descr) in _Operators:
    Operator.__dict__[op] = op
    # FIXME: Can't change the docstring here. What to do?
    # op.__doc = descr
