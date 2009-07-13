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
determined by tge operator type.  Note these methods do not receive *self*
attribute; they are all class methods.

.. function:: typecheck(<exprs>)

  Optional. Adds additional type-checking supplementing the simple
  +isinstance+ test performed by default. If implemented, it is guarenteed
  that all other class methods will only be called if this check returns True.
  
  Returns: bool - True if the operator applies to these expression types.

.. function:: fold(<exprs>)

  Optional. Implement constant folding. It will only be called with
  ~~Expressions for which ~~isConst() returns True.
  
  Returns: ~~expr.Constant - A new expression object representing the folded
  expression. 

.. function:: type(<exprs>)

  Mandatory. Returns the result type this operator evaluates the operands to.

  Returns: ~~type.Type - An type *instance* defining the result type.

.. function:: codegen(<exprs>)

  Mandatory. Implements HILTI code generation. The generated code must
  correspond to one evaluation of the operator with the given expressions.

  Returns: ~~support.HiltiNode - The root node of the generated HILTI code.

"""

import inspect
import functools

from support import *

### Available operators and operator methods.

_Operators = [
    # Operator name, #operands, description.
    ("Plus", 2, "The sum of two expressions. (`a + b`)"),
    ("Minus", 2, "The difference of two expressions. (`a + b`)"),
    ("Mult", 2, "The product of two expressions. (`a * b`)"),
    ("Div", 2, "The division of two expressions. (`a / b`)"),
    ("Neg", 1, "The negation of an expressions. (`- a`)"),
    ]

_Methods = ["typecheck", "fold", "codegen", "type"]
    
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
    return func(*args) if func else None

def codegen(op, *args):
    """Generates HILTI code for an expression.
    
    This function must only be called if ~~typecheck indicates that operator
    and expressions are compatible. 

    op: string - The name of the operator.
    args: one or more ~~Expressions - The expressions for the operator.
    
    Returns: ~~support.HiltiNode - The root node of the generated HILTI code.
    """
    if not typecheck(op, *args):
        util.error("no matching %s operator for %s" % (op, _fmtArgTypes(args)))

    func = _findOp("codegen", op, args)
    
    if not func:
        util.error("no codegen implementation for %s operator with %s" % (op, _fmtArgTypes(args)))
    
    return func(*args)

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
            self.__dict__[op] = functools.partial(_makeOp, op)

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

# Trigger reading types.    
    
import pactypes
