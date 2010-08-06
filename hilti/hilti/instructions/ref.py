# $Id$
"""
References
~~~~~~~~~~

The ``ref<T>`` data type encapsulates references to dynamically
allocated, garbage-collected objects of type *T*. 

The special reference constant ``Null`` can used as place-holder for
invalid references.  

If not explictly initialized, references are set to ``Null`` initially.
"""

import llvm.core

from hilti import hlt
from hilti import type
from hilti import util
from hilti.constraints import *
from hilti.instruction import *

@hlt.type("ref", 6)
class Reference(type.ValueType, type.Constable, type.Constructable, type.Parameterizable, type.Unpackable):
    """Type for references.
    
    ty: ~~HeapType - The type referenced. The type can be None to indicate the
    wildcard reference ``ref<*>``.
    """
    def __init__(self, ty):
        if ty:
            util.check_class(ty, [type.HeapType, type.Unknown], "Reference.__init__")
        
        super(Reference, self).__init__()
        self._type = ty
       
    def refType(self):
        """Returns the type referenced.
        
        Returns: ~~HeapType - The type referenced, which The type may be None
        to indicate the wildcard reference ``ref<*>``.
        """
        return self._type

    ### Overridden from Type.
    
    def name(self):
        return "ref<%s>" % (self._type if self._type else "*")
    
    def _resolve(self, resolver):
        if self._type:
            self._type = self._type.resolve(resolver)
            
        return self
        
    def _validate(self, vld):
        if self._type:
            self._type.validate(vld)

    def output(self, printer):
        printer.output("ref<")
        printer.printType(self._type)
        printer.output(">")
            
    def cmpWithSameType(self, other):
        return self._type == other._type
    
    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        if not self._type:
            # We only create a type info for the wildcard type as that's the
            # only one we need.
            typeinfo = cg.TypeInfo(self)
            typeinfo.c_prototype = "void *"
            return typeinfo
    
        return None
    
    def llvmType(self, cg):
        """A ``ref<T>`` is mapped to the same type as ``T``. Note that because
        ``T`` must be a ~~HeapType, and all ~~HeapTypes are mapped to
        pointers, ``ref<T>`` will likewise be mapped to a pointer. In
        addition, the type information for ``ref<T>`` will be the type
        information of ``T``. Values of type wildcard reference ``ref<*>``
        will always be passed with type information.
        """
        if self._type:
            return self._type.llvmType(cg)
        else:
            return cg.llvmTypeGenericPointer()

    def cPassTypeInfo(self):
        return not self._type
        
    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, type.Reference):
            return False

        if not self._type:
            return True
        
        if not dsttype._type:
            return True
        
        return type.canCoerceTo(self._type, dsttype.refType())
        
    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)

        if isinstance(dsttype, type.Any):
            return value
        
        if not self._type or not dsttype._type:
            return cg.builder().bitcast(value, cg.llvmTypeGenericPointer())
        
        return type.llvmCoerceTo(cg, value, self._type, dsttype.refType())
        
    ### Overridden from ValueType.
    
    def llvmDefault(self, cg):
        """References are initialized to ``null``."""
        return llvm.core.Constant.null(cg.llvmType(self._type))

    def instantiable(self):
        # Can't do ref<*>
        return self._type != None
    
    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        if const.value():
            util.internal_error("Python value for const ref can only be None but is %s" % const.value())
    
    def canCoerceConstantTo(self, const, dsttype):
        if isinstance(dsttype, type.Any):
            return True
        
        if not isinstance(dsttype, type.Reference):
            return False

        if not const.type().refType():
            return True
        
        if not dsttype.type().refType():
            return True
        
        return False

    def coerceConstantTo(self, cg, const, dsttype):
        if isinstance(dsttype, type.Any):
            return const
        
        return constant.Constant(None, dsttype)

    def llvmConstant(self, cg, const):
        """There is a special ``null`` constant marking an unset reference.
        ``null`` can be converted to any other reference type."""
        # This can only be "null"
        assert const.value() == None
        ltype = cg.llvmType(const.type())
        return llvm.core.Constant.null(ltype)

        # FIXME: What do we need this for?
        #
        #    # Pass on to ctor of referenced type.
        #    #
        #    # FIXME: Write a function for doing the callback.
        #    return cg._callCallback(cg._CB_CTOR_EXPR, op.type().refType(), [op, coerce_to])
    
    def outputConstant(self, printer, const):
        # This can only be "null"
        assert const.value() == None
        printer.output("Null")

    ### Overridden from Constructable.

    def validateCtor(self, vld, value):
        assert isinstance(self._type, type.Constructable)
        self._type.validateCtor(vld, value)

    def canCoerceCtorTo(self, ctor, dsttype):
        if isinstance(dsttype, type.Any):
            return True
        
        if not isinstance(dsttype, type.Reference):
            return False

        if not dsttype.refType():
            return True
        
        return self.refType().canCoerceCtorTo(ctor, dsttype.refType())

    def coerceCtorTo(self, cg, ctor, dsttype):
        if isinstance(dsttype, type.Any):
            return ctor
        
        return self.refType().coerceCtorTo(cg, ctor, dsttype.refType())
        
    def llvmCtor(self, cg, value):
        assert isinstance(self._type, type.Constructable)
        return self._type.llvmCtor(cg, value)
    
    def outputCtor(self, printer, val):
        assert isinstance(self._type, type.Constructable)
        self._type.outputCtor(printer, val)
    
    ### Overridden from Parameterizable.
    
    def args(self):
        return [self._type]

    ### Overriden from Unpackable.

    # These just forward to the referenced type.
    
    def formats(self, mod):
        assert isinstance(self._type, type.Unpackable)
        return self._type.formats(mod)
        
    def llvmUnpack(self, cg, begin, end, fmt, arg):
        assert isinstance(self._type, type.Unpackable)
        return self._type.llvmUnpack(cg, begin, end, fmt, arg)
    
@hlt.instruction("ref.cast.bool", op1=cReference, target=cBool)
class CastBool(Instruction):
    """
    Converts *op1* into a boolean. The boolean's value will be True if *op1*
    references a valid object, and False otherwise. 
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        voidp = cg.builder().bitcast(op1, cg.llvmTypeGenericPointer())
        null = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, voidp, null)
        cg.llvmStoreInTarget(self, result)
