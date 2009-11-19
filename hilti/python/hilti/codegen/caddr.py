# $Id$
#
# Code generator for caddr instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """ A ``caddr`` is mapped to ``void *``."""

@codegen.typeInfo(type.CAddr)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "void *"
    typeinfo.to_string = "hlt::caddr_to_string";
    return typeinfo

@codegen.llvmDefaultValue(type.CAddr)
def _(type):
    return llvm.core.Constant.null(codegen.llvmTypeGenericPointer())

@codegen.llvmType(type.CAddr)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.when(instructions.caddr.Function)
def _(self, i):
    
    fid = i.op1().value()
    func = self.lookupFunction(fid)
    builder = self.builder()

    name = self.nameFunction(func, prefix=False)
    main = self._llvm.module.get_function_named(name)
    main = builder.bitcast(main, self.llvmTypeGenericPointer())
        
    if func.callingConvention() == function.CallingConvention.HILTI:
        resume = self._llvm.module.get_function_named(name + "_resume")
        resume = builder.bitcast(resume, self.llvmTypeGenericPointer())
    else:
        resume = llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
        
    struct = llvm.core.Constant.struct([main, resume])
    codegen.llvmStoreInTarget(i.target(), struct)
