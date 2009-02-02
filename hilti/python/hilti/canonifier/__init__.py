# $Id$
"""The Canonifier transforms an |ast| into a canonified form suitable for
passing on to :meth:`~hilti.codegen.generateLLVM`. The canonofied ensure
a set of properties that make it simply the code generation, such as enforcing
a fully linked block structure and replacing some generic instructions with
more specific ones. 

In the future, the Canonifier will also be used to implement some "syntactic
sugar" of which the code generator will not need to be aware of. 
"""

import canonifier
import module
import flow

def canonifyAST(ast):
    """Canonifies the AST *ast* in place."""
    return canonifier.canonifier.canonifyAST(ast)


