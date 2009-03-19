# $Id$
#
# Code generator for string instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

def _llvmStringType(len=0):
    return llvm.core.Type.packed_struct([llvm.core.Type.int(32), llvm.core.Type.array(llvm.core.Type.int(8), len)])

def _llvmStringTypePtr(len=0):
    return llvm.core.Type.pointer(_llvmStringType(len))

def _makeLLVMString(s):
    s = s.encode("utf-8")
    size = llvm.core.Constant.int(llvm.core.Type.int(32), len(s))
    bytes = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in s]
    struct = llvm.core.Constant.packed_struct([size, llvm.core.Constant.array(llvm.core.Type.int(8), bytes)])
        
    name = codegen.nameNewConstant("string")
    glob = codegen.llvmCurrentModule().add_global_variable(_llvmStringType(len(s)), name)
    glob.global_constant = True
    glob.initializer = struct
    glob.linkage = llvm.core.LINKAGE_INTERNAL
        
    # We need to cast the const, which has a specific array length, to the
    # actual string type, which has unspecified array length. 
    return codegen.builder().bitcast(glob, _llvmStringTypePtr(), name)

@codegen.makeTypeInfo(type.String)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::string_to_string";
    return typeinfo

@codegen.defaultInitValue(type.String)
def _(type):
    # A null pointer is treated as the empty string by the C functions.
    return llvm.core.Constant.null(_llvmStringTypePtr(0))
    
@codegen.convertCtorExprToLLVM(type.String)
def _(op, refine_to):
    return _makeLLVMString(op.value())
    
@codegen.convertTypeToLLVM(type.String)
def _(type, refine_to):
    return _llvmStringTypePtr()

@codegen.convertTypeToC(type.String)
def _(type, refine_to):
    """A ``string`` is mapped to a ``__hlt_string *``. The type is defined in
    |hilti_intern.h|:
    
    .. literalinclude:: /libhilti/hilti_intern.h
       :start-after: %doc-hlt_string-start
       :end-before:  %doc-hlt_string-end
       
    """
    return codegen.llvmTypeConvert(type, refine_to)

@codegen.when(instructions.string.Assign)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    self.llvmStoreInTarget(i.target(), op1)
    
@codegen.when(instructions.string.Length)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::string_len", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

import sys
    
@codegen.when(instructions.string.Concat)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::string_concat", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)
    

    



