# $Id$
#
# Code generator for bool instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertToLLVM(type.Bool)
def _(type):
    return llvm.core.Type.int(1)

