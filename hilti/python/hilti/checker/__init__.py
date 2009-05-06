# $Id$
"""Validates the semantic correctness of an |ast|. Inside the
compiler, the checker is the primary component reporting semantic
errors to the user. With the exception of the :mod:`~hilti.parser`
(which reports all syntax errors as well as some semantic errors
directly when reading an input file), other components out-source
their error checking to the checker and generally just double-check
any |ast| properties they require with assertions.  
"""

__all__ = []

import flow
import operators
import checker
import module
import integer

def checkAST(ast):
    """Verifies the semantic correctness of the |ast| *ast*. If any
    errors are detected, suitable error messages are output via
    ~~util. This method is the primary way of ensuring that a HILTI
    |ast| is well-formed, and it should be called whenever an |ast|
    can potentially violate any semantic constraints being relied
    upon by other components. In particular, an |ast| should be
    checked when returned from ~~parse and ~~canonifyAST.
    
    ast: ~~Node - The root of the |ast| to be verified. 
    
    Returns: int - The number of errors detected; zero means
    everything is fine. 
    """
    return checker.checker.checkAST(ast)
