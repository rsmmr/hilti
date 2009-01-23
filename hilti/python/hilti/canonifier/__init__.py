# $Id$

import canonifier
import module
import flow

def canonifyAST(ast):
    """Canonifies *ast* in place"""
    return canonifier.canonifier.canonifyAST(ast)


