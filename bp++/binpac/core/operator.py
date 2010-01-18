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
  
.. function:: validate(vld, <exprs>)

  Optional. Adds additional validation functionality performed *after* it has
  already been determined that the operator applies.

  vld: ~~Validator - The validator to use.

.. function:: fold(<exprs>)

  Optional. Implement constant folding.
  
  Returns: ~~expr.Constant - A new expression object representing the folded
  expression, or None if it can't folded into a constant. 

.. function:: type(<exprs>)

  Mandatory. Returns the result type this operator evaluates the operands to.

  Returns: ~~type.Type - An type *instance* defining the result type.

.. function:: evaluate(cg, <exprs>)

  Mandatory. Implements evaluation of the operator, generating the
  corresponding HILTI code. 

  *cg*: ~~CodeGen - The current code generator.

  Returns: ~~hilti.core.instructin.Operand - An operand with the
  value of the evaluated expression. 

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
   
.. function:: castNonConstExprTo(cg, expr, dsttype):
   
   Optional. Casts a non-constant expression of the operator's type to another
   type. This function will only be called if the corresponding
   ~~canCastNonConstExprTo indicates that the cast is possible. 
        
   *cg*: ~~CodeGen - The current code generator.
   *expr*: ~~Expression - The expression to cast. 
   *dsttype*: ~~Type - The type to cast the expression into.
        
   Returns: ~~Expression - A new expression of the target type with
   the casted expression.
"""

import inspect
import functools

import type as mod_type
import expr

import binpac.support.util as util

### Available operators and operator methods.

def _pacUnary(op):
    def _pac(printer, exprs):
        printer.output(op)
        exprs[0].pac(printer)

    return _pac    
    
def _pacBinary(op):
    def _pac(printer, exprs):
        exprs[0].pac(printer)
        printer.output(op)
        exprs[1].pac(printer)
        
    return _pac    

def _pacEnclosed(op):
    def _pac(printer, exprs):
        printer.output(op)
        exprs[0].pac(printer)
        printer.output(op)

    return _pac    

def _pacMethodCall(printer, exprs):
    exprs[0].pac(printer)
    printer.output(".%s" % exprs[1])
    printer.output("(")
    
    args = exprs[2]
    
    for i in range(len(args)):
        args[i].pac(printer)
        if i != len(args) - 1:
            printer.output(",")
            
    printer.output(")")
    
_Operators = {
    # Operator name, #operands, description.
    "Plus": (2, "The sum of two expressions. (`a + b`)", _pacBinary(" + ")),
    "Minus": (2, "The difference of two expressions. (`a - b`)", _pacBinary(" + ")),
    "Mult": (2, "The product of two expressions. (`a * b`)", _pacBinary(" + ")),
    "Div": (2, "The division of two expressions. (`a / b`)", _pacBinary(" + ")),
    "Neg": (1, "The negation of an expressions. (`- a`)", _pacUnary("-")),
    "Cast": (1, "Cast into another type.", lambda p, e: p.output("<Cast>")),
    "Attribute": (2, "Attribute expression. (`a.b`)", _pacBinary(".")),
    "MethodCall": (3, "Method call. (`a.b()`)", _pacMethodCall),
    "Size": (1, "The size of an expression's value, with type-defined definition of \"size\" (`|a|`)", _pacUnary("|")),
    "Equal": (2, "Compares two values whether they are equal. (``a == b``)", _pacBinary(" + ")),
    "Not": (2, "Inverts a boolean value. (``! a``)", _pacUnary("-"))
    }

_Methods = ["typecheck", "validate", "fold", "evaluate", "type", "castConstTo", "canCastNonConstExprTo", "castNonConstExprTo"]
    
### Public functions.    
    
def typecheck(op, exprs):
    """Checks whether an operator is compatible with a set of operand
    expressions. 
    
    op: ~~Operator - The operator.
    exprs: list of ~~Expression - The expressions for the operator.
    
    
    """
    func = _findOp("typecheck", op, exprs)
    if not func:
        # Not matching operator found.
        return False

    result = func(*exprs)
    
    if not result:
        return False
    
    return True

def validate(op, vld, exprs):
    """Validates an operator's arguments.
    
    op: ~~Operator - The operator.
    vld: ~~Validator - The validator to use.
    exprs: list of ~~Expression - The expressions for the operator.
    """
    func = _findOp("validate", op, exprs)
    if not func:
        # No validate function defined means ok.
        return None
    
    return func(vld, *exprs)
    
def fold(op, exprs):
    """Evaluates an operator with constant expressions.
    
    op: ~~Operator - The operator.
    exprs: list of ~~Expression - The expressions for the operator.
    
    Returns: ~~expr.Constant - An new expression, representing the folded
    expression; or None if the operator does not support constant folding.
    
    Note: This function must only be called if ~~typecheck indicates that
    operator and expressions are compatible. 
    """
    if not typecheck(op, exprs):
        return None

    func = _findOp("fold", op, exprs)
    result = func(*exprs) if func else None
    
    assert (not result) or isinstance(result, expr.Constant)
    return result

def evaluate(op, cg, exprs):
    """Generates HILTI code for an expression.
    
    This function must only be called if ~~typecheck indicates that operator
    and expressions are compatible. 

    op: string - The name of the operator.
    *cg*: ~~CodeGen - The current code generator.

    exprs: list of ~~Expression - The expressions for the operator.

    Returns: ~~hilti.core.instructin.Operand - An operand with the value of
    the evaluated expression. 
    """
    
    if not typecheck(op, exprs):
        util.error("no matching %s operator for %s" % (op, _fmtArgTypes(exprs)))

    func = _findOp("evaluate", op, exprs)
    
    if not func:
        util.error("no evaluate implementation for %s operator with %s" % (op, _fmtArgTypes(exprs)))
    
    return func(cg, *exprs)

def type(op, exprs):
    """Returns the result type for an operator.
    
    This function must only be called if ~~typecheck indicates that operator
    and expressions are compatible.

    op: string - The name of the operator.
    exprs: list of ~~Expression - The expressions for the operator.
    
    Returns: ~~type.Type - The result type.
    """
    if not typecheck(op, exprs):
        util.error("no matching %s operator for %s" % (op, _fmtArgTypes(exprs)))

    func = _findOp("type", op, exprs)
    
    if not func:
        util.error("no type implementation for %s operator with %s" % (op, _fmtArgTypes(exprs)))

    return func(*exprs)

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
    
def castNonConstExprTo(cg, expr, dsttype): 
    """
    Casts a non-constant expression of one type into another. This operator must only
    be called if ~~canCastNonConstExprTo indicates that the cast is supported. 
    
    *cg*: ~~CodeGen - The current code generator.
    expr: ~~expr - The expression to cast. 
    dsttype: ~~Type - The type to cast the expression into. 
    
    Returns: ~~Expression - A new expression of the target type with the
    casted expression.
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
    
    return func(cg, expr, dsttype)
    

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
    for (op, vals) in _Operators.items().sorted():
        (exprs, descr, pac) = vals
        exprs = ", ".join(["type"] * exprs)
        print >>file, ".. function @operator.%s(%s)" % (op, exprs)
        print >>file
        print >>file, "   %s" % descr
        print >>file

def pacOperator(printer, op, exprs):
    """Converts an operator into parseable BinPAC++ code.

    printer: ~~Printer - The printer to use.
    op: string - The name of the operator; use the operator.Operator.*
    constants here.
    exprs: list of ~Expression - The operator's operands.
    """
    vals = _Operators[op]
    (num, descr, pac) = vals
    pac(printer, exprs)
        
### Internal code for keeping track of registered operators.

_OverloadTable = {}

def _registerOperator(operator, f, exprs):
    try:
        _OverloadTable[operator] += [(f, exprs)]
    except KeyError:
        _OverloadTable[operator] = [(f, exprs)]

def _default_typecheck(*exprs):
    return True

def _fmtTypes(types):
    return ",".join([str(t) for t in types])

def _fmtArgTypes(exprs):
    return " ".join([str(a.type() if isinstance(a, mod_type.Type) else a) for a in exprs])

def _makeOp(op, *exprs):
    def __makeOp(cls):
        for m in _Methods:
            if m in cls.__dict__:
                f = cls.__dict__[m]
                #if len(inspect.getexprspec(f)[0]) != len(exprs):
                #    util.internal_error("%s has wrong number of argument for %s" % (m, _fmtTypes(exprs)))
                
                _registerOperator((op, m), f, exprs)

        if not "typecheck" in cls.__dict__:
            # Add a "always true" typecheck if there's no other defined.
            _registerOperator((op, "typecheck"), _default_typecheck, exprs)
                
        return cls
    
    return __makeOp
    
class _Decorators:    
    def __init__(self):
        for (op, vals) in _Operators.items():
            (exprs, descr, pac) = vals
            globals()[op] = functools.partial(_makeOp, op)

class Mutable:
    def __init__(self, arg):
        self._arg = arg

def _matchExpr(expr, proto, all):
    if isinstance(proto, list) or isinstance(proto, tuple):
        if not (isinstance(expr, list) or isinstance(expr, tuple)):
            return False
        
        if len(proto) != len(expr):
            return False
        
        for (e, p) in zip(expr, proto):
            if not _matchExpr(e, p, all):
                return False
            
        return True

    if inspect.isfunction(proto):
        proto = proto(all)

    mutable = False
    if isinstance(proto, Mutable):
        mutable = True
        proto = proto._arg
        
    if inspect.isclass(proto):
        return isinstance(expr.type(), proto) and (not mutable or not expr.isConst())

    if isinstance(proto, mod_type.Type):
        return proto == expr.type() and (not mutable or not expr.isConst())

    return expr == proto
            
def _findOp(method, op, exprs):
    assert method in _Methods
    
    try:
        ops = _OverloadTable[(op, method)]
    except KeyError:
        return None

    matches = []

    for (func, types) in ops:
        
        if len(types) != len(exprs):
            continue
        
        for (a, t) in zip(exprs, types):
            if not _matchExpr(a, t, exprs):
                break
        else:
            matches += [(func, types)]

    if not matches:
        return None
            
    if len(matches) == 1:
        return matches[0][0]

    msg = "ambigious operator definitions: types %s match\n" % _fmtArgTypes(exprs)
    for (func, types) in matches:
        msg += "    %s\n" % _fmtTypes(types)
        
    util.internal_error(msg)

### Initialization code.

# Create "operator" namespace.
operator = _Decorators()            

# Add operator constants to class Operators.
for (op, vals) in _Operators.items():
    (exprs, descr, pac) = vals
    Operator.__dict__[op] = op
    # FIXME: Can't change the docstring here. What to do?
    # op.__doc = descr
