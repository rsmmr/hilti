# $Id$
#
# Code generator for regexp instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """A ``regexp`` object is mapped to ``hlt_regexp *``."""

@codegen.typeInfo(type.RegExp)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_regexp *"
    typeinfo.to_string = "hlt::regexp_to_string"
    return typeinfo

@codegen.llvmType(type.RegExp)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.llvmCtorExpr(type.RegExp)
def _(op, refine_to):
    # We create a global that keeps the regular expressions.  Note that the
    # global needs to be dynamically intialized via an LLVM ctor. 
    ptr = llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
    const = codegen.llvmAddGlobalVar(ptr, "regexp-const")

    def callback():
        regexp = codegen.llvmGenerateCCallByName("hlt::regexp_new", [], abort_on_except=True)
        ref_type = type.Reference([type.RegExp()])

        if len(op.value()) == 1:
            # Just one pattern. We use regexp_compile().
            arg = instruction.ConstOperand(constant.Constant(op.value()[0], type.String()))
            codegen.llvmGenerateCCallByName("hlt::regexp_compile", [regexp, codegen.llvmOp(arg)], 
                                            arg_types = [ref_type, type.String()], 
                                            llvm_args=True, abort_on_except=True)
            
        else:
            # More than one pattern. We built a list of the patterns and the
            # call compile_set. 
            item_type = type.String()
            list_type = type.Reference([type.List([item_type])])
            
            list = codegen.llvmGenerateCCallByName("hlt::list_new", [instruction.TypeOperand(item_type)], abort_on_except=True)
            
            for pat in op.value():
                item = instruction.ConstOperand(constant.Constant(pat, item_type))
                codegen.llvmGenerateCCallByName("hlt::list_push_back", [list, codegen.llvmOp(item)], 
                                                arg_types=[list_type, item_type], llvm_args=True, abort_on_except=True)

            codegen.llvmGenerateCCallByName("hlt::regexp_compile_set", [regexp, list], 
                                            arg_types = [ref_type, list_type], 
                                            llvm_args=True, abort_on_except=True)
            
        codegen.llvmAssign(regexp, const)

    codegen.llvmAddGlobalCtor(callback)
    
    return const

@codegen.operator(instructions.regexp.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::regexp_new", [])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.regexp.Compile)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        self.llvmGenerateCCallByName("hlt::regexp_compile", [i.op1(), i.op2()])
    else:
        self.llvmGenerateCCallByName("hlt::regexp_compile_set", [i.op1(), i.op2()])
    
@codegen.when(instructions.regexp.Find)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("hlt::regexp_string_find", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("hlt::regexp_bytes_find", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.regexp.Span)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("hlt::regexp_string_span", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("hlt::regexp_bytes_span", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.regexp.Groups)
def _(self, i):
    if isinstance(i.op2().type(), type.String):
        # String version.
        result = self.llvmGenerateCCallByName("hlt::regexp_string_groups", [i.op1(), i.op2()])
    else:
        # Bytes version.
        result = self.llvmGenerateCCallByName("hlt::regexp_bytes_groups", [i.op1(), i.op2(), i.op3()])
    
    self.llvmStoreInTarget(i.target(), result)
