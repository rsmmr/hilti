# $Id$
#
# Code generator for bool instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertConstToLLVM(type.Bool)
def _(op, cast_to):
    assert not cast_to_type or cast_to_type == op.type()
    return llvm.core.Constant.int(llvm.core.Type.int(1), 1 if op.value() else 0)

@codegen.convertTypeToLLVM(type.Bool)
def _(type):
    return llvm.core.Type.int(1)



