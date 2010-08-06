# $Id$
"""
Tuples
~~~~~~

The *tuple* data type represents ordered tuples of other values, which can be
of mixed type. Tuple are however statically types and therefore need the
individual types to be specified as type parameters, e.g.,
``tuple<int32,string,bool>`` to represent a 3-tuple of ``int32``, ``string``,
and ``bool``. Tuple values are enclosed in parentheses with the individual
components separated by commas, e.g., ``(42, "foo", True)``.  If not explictly
initialized, tuples are set to their components' default values initially.
"""

import llvm.core

import hilti.operand as operand
import hilti.type as type

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("tuple", 5)                
class Tuple(type.ValueType, type.Constable, type.Parameterizable):
    """A type for tuples of values. 
    
    types: list of ~~Type - The types of the individual tuple elements, or an
    empty list for ``tuple<*>``."""
    def __init__(self, args):
        super(Tuple, self).__init__()
        self._types = args
            
    def types(self):
        """Returns the types of the typle elements.
        
        Returns: list of ~~ValueType - The types, which may be empty for
        ``tuple<*>``.
        """
        return self._types

    def setTypes(self, types):
        """Sets the types of the typle elements.
        
        types: list of ~~ValueType - The types.
        """
        self._types = types

    ### Overridden from Type.
    
    def name(self):
        if self._types:
            return "tuple<%s>" % (",".join([t.name() for t in self._types]))
        else:
            return "tuple<*>"

    def _validate(self, vld):
        for t in self._types:
            t.validate(vld)
        
    def _resolve(self, resolver):
        self._types = [t.resolve(resolver) for t in self._types]
        return self

    def cmpWithSameType(self, other):
        if not self._types:
            return True
        
        return self._types == other._types

    ### Overridden from HiltiType.
    
    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::tuple_to_string";
        typeinfo.hash = "hlt::tuple_hash"
        typeinfo.equal = "hlt::tuple_equal"
        
        # Calculate the offset array. 
        zero = cg.llvmGEPIdx(0)
        null = llvm.core.Constant.null(llvm.core.Type.pointer(self._tupleType(cg)))
        
        offsets = []
        for i in range(len(self._types)):
            idx = cg.llvmGEPIdx(i)
            # This is a pretty awful hack but I can't find a nicer way to
            # calculate the offsets as *constants*, and this hack is actually also
            # used by LLVM internaly to do sizeof() for constants so it can't be
            # totally disgusting. :-)
            offset = null.gep([zero, idx]).ptrtoint(llvm.core.Type.int(16))
            offsets += [offset]
    
        name = cg.nameTypeInfo(self) + "_offsets"
        const = llvm.core.Constant.array(llvm.core.Type.int(16), offsets)
        glob = cg.llvmNewGlobalConst(name, const)
        glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY
    
        typeinfo.aux = glob
        
        return typeinfo

    def llvmType(self, cg):
        """
        A ``tuple<type1,type2,...>`` is mapped to a C struct that consists of fields of the
        corresponding type. For example, a ``tuple<int32, string>`` is mapped to a
        ``struct { int32_t f1, hlt_string* f2 }``.
        
        A tuple's type-information keeps additional layout information in the ``aux``
        field: ``aux`` points to an array of ``int16_t``, one for each field. Each
        array entry gives the offset of the corresponding field from the start of the
        struct. This is useful for C functions that work with tuples of arbitrary
        types as they otherwise would not have any portable way of addressing the
        individual fields. 
        
        Values of type wildcard tuple ``tuple<*>`` will always be passed with
        type information.
        """
        return self._tupleType(cg)

    def cPassTypeInfo(self):
        return not self._types
    
    def llvmDefault(self, cg):
        t = self._tupleType(cg)
        consts = [cg.llvmConstDefaultValue(t) for t in self._types]
        return llvm.core.Constant.struct(consts)

    def instantiable(self):
        # Can't do tuple<*>
        return self._types != []
    
    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, Tuple):
            return False
        
        if not dsttype._types:
            return True
        
        if not self._types:
            return False
 
        if len(self._types) != len(dsttype._types):
            return False
            
        for (t1, t2) in zip(self._types, dsttype._types):
            if not type.canCoerceTo(t1, t2):
                return False
            
        return True

    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)
        
        if isinstance(dsttype, type.Any):
            return value

        if self == dsttype:
            return value
        
        if not dsttype._types:
            return value

        new_elems = []
        for i in range(len(self._types)):
            idx = cg.llvmGEPIdx(i)
            ptr = cg.builder().gep(value, [self.llvmGEPIdx(0), idx])
            elem = self.builder().load(ptr)
            elem = type.llvmCoerceTo(cg, elem, self._types[i], dsttype._types[i])
            new_elems += [elem]
            
        return llvm.core.Constant.struct(elems)
        
    ### Overridden from Constable.
    
    def resolveConstant(self, resolver, const):
        types = []
        for op in const.value():
            op.resolve(resolver)
            types += [op.type()]

        const.setType(Tuple(types))
            
    def validateConstant(self, vld, const):
        for op in const.value():
            if not isinstance(op, operand.Operand):
                return False
            
        return True
            
    def llvmConstant(self, cg, const):
        t = const.type()._tupleType(cg)
        struct = cg.llvmAlloca(t)
    
        length = len(const.type()._types)
        vals = const.value()
        
        for i in range(length):
            dst = cg.builder().gep(struct, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(i)])
            cg.builder().store(cg.llvmOp(vals[i]), dst)
            
        return cg.builder().load(struct)

    def canCoerceConstantTo(self, const, dsttype):
        if not isinstance(dsttype, Tuple):
            return False
        
        if not dsttype._types:
            return True
        
        if not self._types:
            return False

        if len(self._types) != len(dsttype._types):
            return False

        for (op, t) in zip(const.value(), dsttype._types):
            if op.type() == t:
                continue

            c = op.constant()
            if not c:
                # We can only coerce constants.
                return False
            
            if not type.canCoerceConstantTo(c, t):
                return False
            
        return True
        
    def coerceConstantTo(self, cg, const, dsttype):
        assert self.canCoerceConstantTo(const, dsttype)
        
        if isinstance(dsttype, type.Any):
            return const

        if not dsttype._types:
            return const

        new_const = []
        for (op, t) in zip(const.value(), dsttype._types):
            if op.type() == t:
                new_const += [op]
                continue

            c = op.constant()
            assert c
            
            c = type.coerceConstantTo(cg, c, t)
            new_const += [operand.Constant(c)]
            
        return constant.Constant(new_const, dsttype)

    def outputConstant(self, printer, const):
        printer.output("(")
        first = True
        for op in const.value():
            if not first:
                printer.output(", ")
            op.output(printer)
            first = False
        printer.output(")")

    ### Overridden from Parameterizable.

    def args(self):
        return self._types
        
    ### Private.
    
    def _tupleType(self, cg):
        llvm_types = [cg.llvmType(t) for t in self._types]
        return llvm.core.Type.struct(llvm_types)
    
@hlt.constraint("any")
def _isElementIndex(ty, op, i):
    if not op or not isinstance(op, operand.Constant):
        return (False, "must be a constant")

    if not isinstance(ty, type.Integer) or not op.canCoerceTo(type.Integer(32)):
        return (False, "must be of type int<32> but is %s" % ty)
    
    idx = op.value().value()
    if idx >= 0 and idx < len(i.op1().type()._types):
        return (True, "")
    
    return (False, "is out of range")

@hlt.constraint("any")
def _hasElementType(ty, op, i):
    t = i.op1().type()._types[i.op2().value().value()]
    
    if t == ty:
        return (True, "")
    
    return (False, "type must be %s but is %s" % (t, ty))

@hlt.instruction("tuple.index", op1=cTuple, op2=_isElementIndex, target=_hasElementType)
class Index(Instruction):
    """ Returns the tuple's value with index *op2*. The index is zero-based.
    *op2* must be an integer *constant*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2(), coerce_to=type.Integer(32))
        result = cg.llvmExtractValue(op1, op2)
        cg.llvmStoreInTarget(self, result)
    
    
        
