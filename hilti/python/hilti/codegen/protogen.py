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

    def addOutput(self, line):
        self._lines += [line]        
        
    def generateCPrototypes(self, ast, fname):
        """See ~~generateCPrototypes."""
        self._output = open(fname, "w")
        self._module = None
        self._lines = []

        ifdefname = "HILTI_" + os.path.basename(fname).upper().replace("-", "_").replace(".", "_")
        
        print >>self._output, """
/* Automatically generated. Do not edit. */
        
#ifndef %s
#define %s

#include <hilti.h>
""" % (ifdefname, ifdefname)
        
        self.visit(ast)

        for line in sorted(self._lines):
            print >>self._output, line
        
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
                if isinstance(a.type(), type.Reference):
                    ti = cg.getTypeInfo(a.type().refType())
                else:
                    ti = cg.getTypeInfo(a.type())
                assert ti and ti.c_prototype
                args += [ti.c_prototype]

        self.addOutput("%s %s(%s, const __hlt_exception *);" % (result, cg.nameFunction(f, prefix=False), ", ".join(args)))

@protogen.when(id.ID, type.ValueType)
def _(self, i):
   if i.role() == id.Role.CONST:

       # Don't generate constants for recusively imported IDs.
       if i.scope() != self._module.name():
           return
       
       value = self._module.lookupIDVal(i)

       scope = i.scope()
       scope = scope[0].upper() + scope[1:]
       
       self.addOutput("static const int8_t %s_%s = %s;" % (scope, i.name().replace("::", "_"), value))
    
    








