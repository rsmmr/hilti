# $Id$
"""
Booleans
~~~~~~~~

The *bool* data type represents boolean values. The two boolean constants are
``True`` and ``False``. If not explictly initialized, booleans are set to
``False`` initially.

"""

import llvm.core

from hilti import hlt
from hilti import type
from hilti.constraints import *
from hilti.instruction import *

@hlt.type("bool", 3)
class Bool(type.ValueType, type.Constable, type.Unpackable):
    """Type for booleans."""
    def __init__(self, location=None):
        super(Bool, self).__init__(location=location)
        
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``bool`` is mapped to an ``int8_t``, with ``True`` corresponding
        to the value ``1`` and ``False`` to value ``0``."""
        return llvm.core.Type.int(1)

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "int8_t"
        typeinfo.to_string = "hlt::bool_to_string";
        typeinfo.to_int64 = "hlt::bool_to_int64";
        return typeinfo
    
    def llvmDefault(self, cg):
        return cg.llvmConstInt(0, 1)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        return isinstance(const.value(), bool)
    
    def llvmConstant(self, cg, const):
        return cg.llvmConstInt(1 if const.value() else 0, 1)

    def outputConstant(self, printer, const):
        printer.output("True" if const.value() else "False")
    
    ### Overridden from Unpackable.

    def formats(self, mod):
        return [("Bool", type.Integer(8), True, """Reads a
        single byte and, per default, returns ``True` if that byte is non-zero
        and ``False`` otherwise. Optionally, one can specify a particular bit
        (0-7) as additional ``unpack`` arguments and the result will then
        reflect the value of that bit.""")]
        
    def llvmUnpack(self, cg, begin, end, fmt, arg):
        # FIXME: Add error checking. We don't check  the range of the bit
        # number. 
    
        addr = cg.llvmAlloca(cg.llvmType(self))
        iter = cg.llvmAlloca(cg.llvmType(type.IteratorBytes()))

        (val, i) = cg.llvmUnpack(type.Integer(8), begin, end, "Int8")

        builder = cg.builder()
        
        if arg:
            llarg = arg.llvmLoad(cg)
            
            if arg.type().width() > 8:
                llarg = builder.trunc(llarg, llvm.core.Type.int(8))
            
            mask = builder.shl(cg.llvmConstInt(1, 8), llarg)
            val = builder.and_(val, mask)
            
        result = builder.icmp(llvm.core.IPRED_NE, cg.llvmConstInt(0, 8), val)
            
        return (result, i)
        
@hlt.instruction("bool.and", op1=cBool, op2=cBool, target=cBool)
class And(Instruction):
    """
    Computes the logical 'and' of *op1* and *op2*.
    """
    def codegen(self, cg):
        Instruction.codegen(self, cg)
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().and_(op1, op2)
        cg.llvmStoreInTarget(self, result)
    
@hlt.instruction("bool.or", op1=cBool, op2=cBool, target=cBool)
class Or(Instruction):
    """
    Computes the logical 'or' of *op1* and *op2*.
    """
    def codegen(self, cg):
        Instruction.codegen(self, cg)
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().or_(op1, op2)
        cg.llvmStoreInTarget(self, result)
    
@hlt.instruction("bool.not", op1=cBool, target=cBool)
class Not(Instruction):
    """
    Computes the logical 'not' of *op1*.
    """
    def codegen(self, cg):
        Instruction.codegen(self, cg)
        op1 = cg.llvmOp(self.op1())
        result = cg.builder().xor(op1, cg.llvmConstInt(1, 1))
        cg.llvmStoreInTarget(self, result)
    

