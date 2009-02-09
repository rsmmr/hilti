# $Id$

__all__ = []

import codegen
import flow
import integer
import module
import bool

def generateLLVM(ast, verify=True):
    """Compiles *ast* into LLVM module. *ast* must have been already canonified."""
    return codegen.codegen.generateLLVM(ast, verify)


