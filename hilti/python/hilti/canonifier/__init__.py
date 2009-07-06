# $Id$
"""Transforms an |ast| into a canonified form suitable for code generation.
The canonified form ensures a set of properties that make simply the code
generation process, such as enforcing a fully linked block structure and
replacing some generic instructions with more specific ones. Once an |ast| has
been canonified, it can be passed on to :meth:`~hilti.codegen.generateLLVM`.

Note: In the future, the canonifier will also be used to implement additional
'syntactic sugar' for the HILTI language, so that we can writing programs in
HILTI more convenient without requiring additional complexity for the code
generator.
"""

__all__ = []

import canonifier
import module
import flow

def canonifyAST(ast, debug=False):
    """Canonifies an |ast| in place. The |ast| must be well-formed as verified
    by ~~checkAST.
    
    ast: ~~Node - The root of |ast|. It will be changed in place.
    debug: bool - If debug is true, the canonifier may insert
    additional code for helping with debugging. 
    """
    return canonifier.canonifier.canonifyAST(ast, debug)


