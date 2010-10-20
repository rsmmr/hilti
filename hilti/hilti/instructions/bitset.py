# $Id$
"""
Bitsets

The ``bitset`` data type groups a set of bits together. In each instance, any
of the bits is either set or not set. The individual bits are identified by
labels, which are declared in the bitsets type declaration::

   bitset MyBits { Bit1, Bit2, Bit3 }

The individual labels can then be used with the ``bitset.*``
commands::

   local mybits: MyBits
   mybits = bitset.set mybits MyBits::Bit2
   
Note: For efficiency reasons, HILTI supports only up to 64 bits per type. 

Todo: We can't create constants with multiple bits set yet.
"""

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("bitset", 19)
class Bitset(type.ValueType, type.Constable):
    def __init__(self, labels, location=None):
        """The ``bitset`` type. 
        
        Returns: dictonary string -> int - The labels mappend to their values.
        If a value is given as None, it will be chosen uniqule automatically.
        """
        super(Bitset, self).__init__(location=location)
        
        self._labels = {}
        next = 0
        for (label, bit) in labels:
            if bit == None:
                bit = next 
                
            next = max(next, bit + 1)
            self._labels[label] = bit

        self._labels_sorted = labels
            
    def labels(self):
        """Returns the bit labels with their corresponding bit numbers.
        
        Returns: dictonary string -> int - The labels mappend to their values.
        """
        return self._labels

    ### Overridden from Type.

    def name(self):
        return "bitset { %s }" % ", ".join(["%s=%d" % (l, self._labels[l]) for (l, v) in self._labels_sorted])
    
    ### Overridden from HiltiType.
    
    def typeInfo(self, cg):
        """An ``bitset``'s type information keeps additional information in
        the ``aux`` field: ``aux`` points to a concatenation of ASCIIZ strings
        containing the label names. 
        """
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "int64_t"
        typeinfo.to_string = "hlt::bitset_to_string";
        typeinfo.to_int64 = "hlt::bitset_to_int64";
        
        # Build the ASCIIZ strings.
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
        """A ``bitset`` is mapped to an ``int64_t``, with one bit in that
        value corresponding to each defined label."""
        return llvm.core.Type.int(64)

    def _validate(self, vld):
        for bit in self._labels.values():
            if bit >= 64:
                vld.error(self, "bitset can only store bits in the range 0..63")

    def cmpWithSameType(self, other):
        return self._labels == other._labels
        
    ### Overridden from ValueType.
    
    def llvmDefault(self, cg):
        """In a ``bitset``, all bits are initially unitialized."""
        return cg.llvmConstInt(0, 64)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        """``bitset`` constants are strings naming one of the labels."""
        
        label = const.value()
        
        if not isinstance(label, str):
            util.internal_error("bitset label must be a Python string")

        if not label in self._labels:
            vld.error(self, "unknown bitset label %s" % label)

    def llvmConstant(self, cg, const):
        label = const.value()

        bit = self._labels[label]
        return cg.llvmConstInt(1, 64).shl(cg.llvmConstInt(bit, 64))
        
    def outputConstant(self, printer, const):
        printer.output(const.value())

@hlt.overload(Equal, op1=cBitset, op2=cSameTypeAsOp(1), target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bitset.set", op1=cBitset, op2=cSameTypeAsOp(1), target=cSameTypeAsOp(1))
class Set(Instruction):
    """
    Adds the bits set in *op2* to those set in *op1* and returns the result.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().or_(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bitset.clear", op1=cBitset, op2=cSameTypeAsOp(1), target=cSameTypeAsOp(1))
class Clear(Instruction):
    """
    Removes the bits set in *op2* from those set in *op1* and returns the result.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        op2 = cg.builder().xor(op2, cg.llvmConstInt(-1, 64))
        result = cg.builder().and_(op1, op2)
        cg.llvmStoreInTarget(self, result)
    
@hlt.instruction("bitset.has", op1=cBitset, op2=cSameTypeAsOp(1), target=cBool)
class Has(Instruction):
    """
    Returns true if all bits in *op2* are set in *op1*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        tmp = cg.builder().and_(op1, op2)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, tmp, op2)
        cg.llvmStoreInTarget(self, result)



    
    
