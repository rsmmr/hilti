#! /usr/bin/env
#
# C prototype geneator.

import os.path

import llvm
import llvm.core 

global_id = id

from hilti.core import *
from hilti import instructions

import codegen

### ProtoGen visitor.

class ProtoGen(visitor.Visitor):
    """Implements C prototype generation."""
    
    def __init__(self):
        super(ProtoGen, self).__init__()
        
    def generateCPrototypes(self, ast, fname):
        """See ~~generateCPrototypes."""
        self._output = open(fname, "w")
        self._moduel = None

        ifdefname = "HILTI_" + os.path.basename(fname).upper().replace("-", "_").replace(".", "_")
        
        print >>self._output, """
/* Automatically generated. Do not edit. */
        
#ifndef %s
#define %s

#include <hilti.h>
""" % (ifdefname, ifdefname)
        
        self.visit(ast)
        
        print >>self._output, """
#endif
"""
        
protogen = ProtoGen()

@protogen.when(module.Module)
def _(self, m):
    self._module = m

@protogen.pre(function.Function)
def _(self, f):
    
    cg = codegen.codegen
    
    if f.linkage() == function.Linkage.EXPORTED and f.callingConvention() == function.CallingConvention.HILTI:
        
        if f.type().resultType() == type.Void:
            result = "void"
        else:
            ti = cg.getTypeInfo(f.type().resultType())
            assert ti.c_prototype
            result = ti.c_prototype
        
        args = []
        for a in f.type().args():
            if a.type() == type.Void:
                args += ["void"]
            else:
                ti = cg.getTypeInfo(a.type())
                assert ti.c_prototype
                args += [ti.c_prototype]
                
        print >>self._output, "%s %s(%s, const __hlt_exception *);" % (result, cg.nameFunction(f, prefix=False), ", ".join(args))
        
@protogen.when(id.ID, type.ValueType)
def _(self, i):
   if i.role() == id.Role.CONST:
       
       value = self._module.lookupIDVal(i.name())
       print >>self._output, "static const int8_t %s = %s;" % (i.name().replace("::", "_"), value)
    
    








