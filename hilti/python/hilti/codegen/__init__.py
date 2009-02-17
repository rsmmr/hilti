# $Id$
"""Compiles an |ast| into a LLVM module."""

__all__ = []

import codegen
import flow
import integer
import module
import bool

def generateLLVM(ast, verify=True):
    """Compiles the |ast| into LLVM module.  The |ast| must be well-formed as
    verified by ~~checkAST, and it must have been canonified by ~~canonifuAST.
    
    *ast*: ~~Node - The root of the |ast| to turn into LLVM.
    *verify*: bool - If true, the correctness of the generated LLVM code will
    be verified via LLVM's internal validator.
    
    Returns: tuple (bool, llvm.core.Module) - If the bool is True, code generation (and
    if *verify* is True, also verification) was successful. If so, the second
    element of the tuple is the resulting LLVM module.
    """
    return codegen.codegen.generateLLVM(ast, verify)


