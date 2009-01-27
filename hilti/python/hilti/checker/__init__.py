# $Id$

import checker
import flow
import module
import integer
import bool

def checkAST(ast):
    """
    Verifies the correctness of the AST *ast*, as returned for
    example by :meth:`hilti.parser.parse`, and returns the number of
    errors found. If any errors are detected, suitable error
    messages are written to standard error.

    This method is the primary way of ensuring that a HILTI AST is
    well-formed, and should be called whenever an AST could violate
    any semantic constraints being relied upon by the :ref:`code
    generator <internals-codegen>`. In particular, ASTs should be
    checked after :ref:`parsing <internals-parser>` and
    :ref:`canonification <internals-canonifier>`. 
    """ 
    
    return checker.checker.checkAST(ast)
