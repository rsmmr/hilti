# $Id$
#
# Code generator for regexp instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """A ``regexp`` object is mapped to ``__hlt_regexp *``."""

@codegen.typeInfo(type.RegExp)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__hlt_regexp *"
    typeinfo.to_string = "__Hlt::regexp_to_string"
    return typeinfo

@codegen.llvmType(type.RegExp)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.operator(instructions.regexp.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::regexp_new", [])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.regexp.Compile)
def _(self, i):
    self.llvmGenerateCCallByName("__Hlt::regexp_compile", [i.op1(), i.op2()])
    
@codegen.when(instructions.regexp.Find)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_string_find", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_bytes_find", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.regexp.Span)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_string_span", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_bytes_span", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.regexp.Groups)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_string_groups", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("__Hlt::regexp_bytes_groups", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)
