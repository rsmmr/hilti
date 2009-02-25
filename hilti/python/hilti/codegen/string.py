# $Id$
#
# Code generator for string instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertConstToLLVM(type.String)
def _(op, cast_to):
    assert not cast_to_type or cast_to_type == op.type()
    return llvm.core.Constant.string(op.value())

@codegen.convertTypeToLLVM(type.String)
def _(type):
    return llvm.core.Type.array(0, llvm.core.Type.int(8))


