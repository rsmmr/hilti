# $Id$
#
# Code generator for Bytes instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """A ``bytes`` object is mapped to ``__Hlt::bytes *``. The
type is defined in |hilti_intern.h|."""

def _llvmIteratorType(cg):
    # struct __hlt_bytes_pos
    return llvm.core.Type.struct([cg.llvmTypeGenericPointer()] * 2)

def _llvmBytesType():
    return codegen.llvmTypeGenericPointer()

@codegen.typeInfo(type.Bytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__Hlt::bytes *"
    typeinfo.to_string = "__Hlt::bytes_to_string"
    return typeinfo

@codegen.typeInfo(type.IteratorBytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__Hlt::bytes_pos"
    return typeinfo

@codegen.llvmType(type.Bytes)
def _(type, refine_to):
    return _llvmBytesType()

@codegen.llvmType(type.IteratorBytes)
def _(type, refine_to):
    return _llvmIteratorType(codegen)

@codegen.llvmDefaultValue(type.IteratorBytes)
def _(type):
    return llvm.core.Constant.struct([llvm.core.Constant.null(codegen.llvmTypeGenericPointer())] * 2)

@codegen.llvmCtorExpr(type.Bytes)
def _(op, refine_to):
    # We create two globals:
    #
    # (1) one for storing the raw characters themselves.
    # (2) one for storing the bytes objects (which will point to (1))
    #
    # Note that (2) needs to be dynamically intialized via an LLVM ctor. 
    
    size = len(op.value())
    array = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in op.value()]
    array = llvm.core.Constant.array(llvm.core.Type.int(8), array)
    
    data = codegen.llvmAddGlobalConst(array, "bytes-data")
    bytes = codegen.llvmAddGlobalConst(llvm.core.Constant.null(_llvmBytesType()), "bytes")

    def callback():
        builder = codegen.builder()
        bytes_new_from_data = codegen.llvmCurrentModule().get_function_named("__hlt_bytes_new_from_data")
        exception = builder.alloca(codegen.llvmTypeException())
        exception = builder.bitcast(exception, llvm.core.Type.pointer(codegen.llvmTypeGenericPointer()))
        datac = builder.bitcast(data, codegen.llvmTypeGenericPointer())
        newobj = builder.call(bytes_new_from_data, [datac, codegen.llvmConstInt(size, 32), exception])
        # FIXME: What do we do if this guy raises an exception?
        # Just ignore it for now ...
        codegen.llvmAssign(newobj, bytes)

    codegen.llvmAddGlobalCtor(callback)
    
    return codegen.builder().load(bytes)
    
@codegen.operator(instructions.bytes.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_new", [], [])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.bytes.IterIncr)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_pos_incr", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.bytes.IterDeref)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_pos_deref", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.bytes.IterEqual)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_pos_eq", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Length)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_len", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Empty)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_empty", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Append)
def _(self, i):
    self.llvmGenerateCCallByName("__Hlt::bytes_append", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])

@codegen.when(instructions.bytes.Sub)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_sub", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Offset)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_offset", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Begin)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_begin", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.End)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_end", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.bytes.Diff)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_pos_diff", [i.op1(), i.op2()], [i.op2().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)
