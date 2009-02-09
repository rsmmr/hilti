# $Id$
"""The Checker package validates the semantic correctness of an
r|ast|. Other components rely on many of the properties asserted by
the Checker. 

Inside the compiler, the Checker is the primary component reporting
semantic errors to the user. With the exception of the
:mod:`~hilti.parser` (which reports syntax errors as well as some
semantic errors directly when reading an input file), other
components out-source their error checking to the Checker and
otherwise just double-check any properties they require with
assertions.  
"""

__all__ = []

import checker
import flow
import module
import integer
import bool

def checkAST(ast):
    """Verifies the semantic correctness of the |ast| *ast* and
    returns the number of errors found. If any errors are detected,
    suitable error messages are written to standard error via the
    :mod:`~hilti.core.util` module. 

    This method is the primary way of ensuring that a HILTI |ast| is
    well-formed, and should be called whenever an |ast| can
    potentially violate any semantic constraints being relied upon
    by other components. In particular, ASTs should be checked when
    returned from :meth:`~hilti.core.parser.parse` and
    :meth:`~hilti.core.canonifier.canonifyAST`.
    """
    return checker.checker.checkAST(ast)
