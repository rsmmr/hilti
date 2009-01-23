# $Id$

import codegen
import flow
import integer
import module

def generateLLVM(ast, verify=True):
    """Compiles *ast* into LLVM module. *ast* must have been already canonified."""
    return codegen.codegen.generateLLVM(ast, verify)


