# $Id$
"""
Enums
~~~~~

The ``enum`` data type represents a selection of unique values, identified by
labels. Enums must be defined in global space:

   enum Foo { Red, Green, Blue }

The individual labels can then be used as global identifiers. In addition to
the given labels, each ``enum`` type also implicitly defines one additional
label called ''Undef''. 

If not explictly initialized, enums are set to
``Undef`` initially.
"""

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("enum", 10)
class Enum(type.ValueType, type.Constable):
    def __init__(self, labels):
        """The enum type. 
        
        labels: list of string - The labels defined for the enum type.
        """
        super(Enum, self).__init__()
        i = 0
        self._labels = {}
        for t in ["Undef"] + labels:
            self._labels[t] = i
            i += 1
            
    def labels(self):
        """Returns the enums labels with their corresponding integer values.
        
        Returns: dictonary string -> int - The labels mappend to their values.
        """
        return self._labels

    ### Overridden from Type.
    
    def name(self):
        labels = [l for l in sorted(self._labels.keys()) if l != "Undef"]
        return "enum { %s }" % ", ".join(labels)
    
    ### Overridden from HiltiType.
    
    def typeInfo(self, cg):
        """An ``enum``'s type information keep additional information in the
        ``aux`` field: ``aux`` points to a concatenation of ASCIIZ strings
        containing the label names, starting with the undefined value and
        ordered in the order of their corresponding integer values."""
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "int8_t"
        typeinfo.to_string = "hlt::enum_to_string";
        typeinfo.to_int64 = "hlt::enum_to_int64";
        
        # Build ASCIIZ strings for the labels.
        aux = []
        zero = [cg.llvmConstInt(0, 8)]
        for (label, value) in sorted(self.labels().items(), key=lambda x: x[1]):
            aux += [cg.llvmConstInt(ord(c), 8) for c in label] + zero
    
        name = cg.nameTypeInfo(self) + "_labels"
        const = llvm.core.Constant.array(llvm.core.Type.int(8), aux)
        glob = cg.llvmNewGlobalConst(name, const)
        glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY
        
        typeinfo.aux = glob
        
        return typeinfo

    def llvmType(self, cg):
        """An ``enum`` is mapped to an ``int8_t``, with a unique integer corresponding to each label."""
        return llvm.core.Type.int(8)
        
    ### Overridden from ValueType.
    
    def llvmDefault(self, cg):
        """``enum`` variables are initialized with their ``Undef`` label."""
        return cg.llvmConstInt(self._labels["Undef"], 8)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        if not isinstance(const.value(), str):
            util.internal_error("enum label must be a Python string")
            
        if not const.value() in self._labels:
            vld.error(self, "undefined enum label %s" % const.value())

    def llvmConstant(self, cg, const):
        return cg.llvmConstInt(self._labels[const.value()], 8)

    def outputConstant(self, printer, const):
        
        for i in printer.currentModule().scope().IDs():
            if not isinstance(i, id.Type):
                continue

            if i.type() == self:
                namespace = i.name()
        
        printer.output(namespace + "::" + const.value())

@hlt.overload(Equal, op1=cEnum, op2=cSameTypeAsOp(1), target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)
        



