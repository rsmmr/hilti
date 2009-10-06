# $Id$
#
# Code generator for the exception type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
An ``exception`` is mapped to an ``hlt_exception *``.
"""

@codegen.typeInfo(type.Exception)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_exception *"
    return typeinfo

@codegen.llvmType(type.Exception)
def _(type, refine_to):
    return codegen.llvmTypeExceptionPtr()

