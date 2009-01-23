# $Id$

import checker
import flow
import module
import integer

def checkAST(ast):
    """Returns number of errors found."""
    return checker.checker.checkAST(ast)
