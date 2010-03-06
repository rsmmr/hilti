# $Id$

builtin_type = type

import llvm.core

import constant
import id
import node
import type
import util

class Operand(node.Node):
    """Base class for operands and targets of HILTI instructions. 
    
    location: ~~Location - A location to be associated with the operand. 
    
    Note: The ~~codegen methods aren't used because we need a number
    of different access models. Use ~~llvmLoad, and ~~llvmStore
    instead.
    """
    def __init__(self, location):
        super(Operand, self).__init__(location)
        self._type = None
        self._coerced = []

    def _setType(self, t):
        """Must be called by derived classes to set the operand's original
        type.
        
        t: ~~Type - The type.
        """
        self._type = t
        
    def type(self):
        """Return's the operand's type. Per default, this is the operand
        orignal type. However, if it has been coerced into something different
        by ~~coerceTo, the new type will be returned.
        
        Returns: ~~Type - The operand's type.
        """
        return self._coerced[-1] if self._coerced else self._type
        
    ### Methods for derived classes to override. 
    
    def value(self):
        """Return's the operand's value. 
        
        Must be overridden by derived classes.
        
        Returns: object - The operand's value. The kind of object is defined
        by the derived class.
        """
        util.internal_error("value() not implemented for %s" % repr(self))

    def constant(self):
        """Returns the constant an operand evaluates to, if any.
        
        Must be overridden by derived classes.
        
        Returns: ~~constant.Constant or None - The constant, or None if the
        operand does not evaluate to anything constant.
        """
        util.internal_error("constant() not implemented for %s" % repr(self))
    
    def canCoerceTo(self, ty):
        """Returns true if the operand can be coerced to the given destination
        type.
        
        ty: ~~Type - The destination type.
        """
        if isinstance(ty, type.Any):
            return True
        
        return type.canCoerceTo(self.type(), ty)

    def coerceTo(self, cg, ty):
        """Coerces the operand to the given destination type. The operand may
        be modified *in place* and will then from now on have the coerced type
        as its ~~type. This method must only be called if ~~canCoerceTo
        indicates that coercion is ok.
        
        cg: ~~CodeGen - The code generator to use.
        
        ty: ~~Type - The destination type.
        
        Returns: ~~Operand - The coerced operand. Note that *self* may be
        modified as well.
        
        Todo: We should remove this in-place modification and always return a
        new operand.
        """
        assert self.canCoerceTo(ty)
        
        if isinstance(ty, type.Any):
            return self
        
        self._coerced += [ty]
        return self

    def llvmLoad(self, cg):
        """Loads the value the operand refers to.

        Must be overridden by derived classes.
        
        cg: The current code generator.
        
        Returns: llvm.core.Value - The value.
        """
        util.internal_error("llvmLoad() not implemented for %s" % repr(self))
        
    def llvmStore(self, cg, val):
        """Stores a value in operand refers to. Not all operands support
        writes.

        Must be overridden by derived classes if it supports writes.
        
        cg: The current code generator.
        
        val: llvm.core.Value - The value to store.
        
        """
        util.internal_error("llvmStore() not implemented for %s" % repr(self))

    def __str__(self):
        return unicode(self.value())
        
    ### Overridden from Node.
    
    def resolve(self, resolver):
        self._type = self._type.resolve(resolver)
    
    def validate(self, vld):
        self._type.validate(vld)
    
    def canonify(self, canonifier):
        pass
    
    def codegen(self, codegen):
        # Don't overide in derived classes.
        util.internal_error("Operands do not implement codegen. Use llvmLoad/Store.")

    # Internal methods.
    
    def _coerce(self, cg, val):
        cur = self._type
        
        for next in self._coerced:
            val = type.llvmCoerceTo(cg, val, cur, next)
            cur = next
        
        assert val
        return val
    
    def _coerceConst(self, const):
        cur = self._type
        
        for next in self._coerced:
            const = type.coerceConstantTo(cg, const, cur, next)
            cur = next
        
        assert const
        return const
        
class Constant(Operand):
    """Represents a constant operand. 
    
    constant: ~~Constant - The constant value.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, constant, location=None):
        super(Constant, self).__init__(location)
        self.setValue(constant)

    def value(self):
        """Returns the constant the operand represents.
        
        Returns: ~~Constant - The constant.
        """
        return self._constant
        
    def setValue(self, const):
        """Sets the operand value.
        
        const: ~~Constant - The new constant.
        """
        util.check_class(const, constant.Constant, "Constant")
        self._constant = const
        self._setType(const.type())

    ### Overridden from Operand.
        
    def type(self):
        return self._constant.type()
    
    def constant(self):
        return self._constant
    
    def canCoerceTo(self, ty):
        return type.canCoerceConstantTo(self._constant, ty)
        
    def coerceTo(self, cg, ty):
        if not self.canCoerceTo(ty):
            util.internal_error("cannot coerce operand of type %s to %s" % (self.type(), ty))

        const = type.coerceConstantTo(cg, self._constant, ty)
        return Constant(const, location=self.location())
    
    def llvmLoad(self, cg):
        return self._constant.llvm(cg)
        
    def llvmStore(self, cg):
        util.internal_error("llvmStore() not possible for Constant")

    ### Overridden from Node.
    
    def resolve(self, resolver):
        super(Constant, self).resolve(resolver)
        self._constant.resolve(resolver)
        self.setValue(self._constant)
    
    def validate(self, vld):
        super(Constant, self).validate(vld)
        self._constant.validate(vld)
    
    def output(self, printer):
        self._constant.output(printer)

    def canonify(self, canonifier):
        super(Constant, self).canonify(canonifier)
        self._constant.canonify(canonifier)
        
        
class Ctor(Operand):
    """Represents an operand created by a non-constant constructor expression.
    
    value: any - The ctor expression's value, with type determined by *ty*.
    
    ty: ~~Type - The type for the ctor expression; must be
    ~~Constructable.
    
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, val, type, location=None):
        super(Ctor, self).__init__(location)
        self.setValue(val, type)

    def value(self):
        """Returns the operand's value.
        
        Returns: any - The value, with it's type determined by ~~type.
        """
        return self._value
        
    def setValue(self, val, ty):
        """Sets the operand value.
        
        val: any - The new value, with its type determined by *ty*.
        ty: ~~Type - The type of the new value, which must be a ~~Constructable.
        """
        util.check_class(ty, type.Constructable, "Operand.Ctor")
        self._value = val
        self._setType(ty)
        self._orig_type = ty
        
    ### Overridden from Operand.
        
    def constant(self):
        return None
    
    def llvmLoad(self, cg):
        return self._coerce(cg, self._orig_type.llvmCtor(cg, self._value))
        
    def llvmStore(self, cg):
        util.internal_error("llvmStore() not possible for Ctor")

    ### Overridden from Node.
    
    def validate(self, vld):
        super(Ctor, self).validate(vld)
        self.type().validateCtor(vld, self._value)
    
    def output(self, printer):
        self.type().outputCtor(printer, self._value)
        
class ID(Operand):
    """Represents an ID operand.
    
    id: ~~ID - The operand's ID.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, id, location=None):
        super(ID, self).__init__(location)
        self.setValue(id)

    def value(self):
        """Returns the ID the operand refers to.
        
        Returns: ~~ID - The id.
        """
        return self._id

    def setValue(self, i):
        """Sets the operand's ID.
        
        i: ~~ID - The new ID.
        """
        assert isinstance(i, id.ID)
        self._id = i
        self._setType(i.type())
    
    ### Overridden from Operand.
        
    def constant(self):
        if isinstance(self._id, id.Constant):
            return self._id.value()
        
        return None
    
    def canCoerceTo(self, ty):
        if isinstance(self._id, id.Constant):
            # Our ID is referencing a constant.
            return type.canCoerceConstantTo(self._id.value(), ty)
        else:
            return super(ID, self).canCoerceTo(ty)

    def coerceTo(self, cg, ty):
        if isinstance(self._id, id.Constant):
            # Our ID is referencing a constant.
            const = type.coerceConstantTo(cg, self._id.value(), ty)
            return Constant(const, location=self.location())
        else:
            return super(ID, self).coerceTo(cg, ty)
        
    def llvmLoad(self, cg):
        if isinstance(self._id, id.Constant):
            # Our ID is referencing a constant.
            const = self._id.value()
            const = Constant(const, location=self.location())
            return const.llvmLoad(cg)
            
        else:
            return self._coerce(cg, self._id.llvmLoad(cg))
        
    def llvmStore(self, cg, val):
        return self._id.llvmStore(cg, val)
            
    ### Overridden from Node.
    
    def resolve(self, resolver):
        super(ID, self).resolve(resolver)
        if isinstance(self._id, id.Unknown):
            i = self._id.scope().lookup(self._id.name())
            if i:
                self._id = i
                
        self._id.resolve(resolver)
        self.setValue(self._id)
    
    def validate(self, vld):
        super(ID, self).validate(vld)
        if isinstance(self._id, id.Unknown):
            if not self._id.scope().lookup(self._id.name()):
                vld.error(self, "unknown identifier %s" % self._id.name())
                return 
            
        self._id.validate(vld)
            
    def output(self, printer):
        if self._id.namespace() and self._id.namespace() != printer.currentModule().name():
            printer.output("%s::%s" % (self._id.namespace(), self._id.name()))
        else:
            printer.output(self._id.name())
    
    def canonify(self, canonifier):
        self._id.canonify(canonifier)


class Type(Operand):
    """Represents a type as an operand. 
     
    t: ~~Type - The type the operand refers to. 
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, t, location=None):
        super(Type, self).__init__(location)
        self.setValue(t)

    def value(self):
        """Returns the type the operand refers to.
        
        Returns: ~~Type - The type.
        """
        return self._value

    def setValue(self, t):
        """Sets the operand's type.
        
        t: ~~Type - The new type.
        """
        assert isinstance(t, type.Type)
        self._value = t
        self._setType(type.MetaType())
        
    ### Overridden from Operand.
        
    def constant(self):
        return None
    
    ### Overridden from Node.
    
    def output(self, printer):
        printer.printType(self._value)

    ### Overridden from HiltiType.
    
    def llvmLoad(self, cg, coerce_to=None):
        ti = cg.llvmTypeInfoPtr(self._value)
        return cg.builder().bitcast(ti, cg.llvmTypeTypeInfoPtr())
        
    def llvmStore(self, cg):
        util.internal_error("llvmStore() not possible for Type")
    
            
class LLVM(Operand):
    """Represents an operand for which we already have an LLVM
    representation. 
     
    v: ~~llvm.core.Value - The LLVM value for the operand.
    t: ~~Type - The operand's original type.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, v, t, location=None):
        super(LLVM, self).__init__(location)
        self.setValue(v, t)

    def value(self):
        """Returns the operand's LLVM value.
        
        Returns: llvm.core.Value - The value.
        """
        return self._val

    def setValue(self, v, t):
        """Sets the operand value and type.
        
        v: ~~llvm.core.Value - The LLVM value for the operand.
        t: ~~Type - The operand's original type.
        """
        if not isinstance(v, llvm.core.Value):
            util.internal_error("LLVM: value must be llvm.core.Value but is %s" % repr(v))
            
        if not isinstance(t, type.Type):
            util.internal_error("LLVM: type must be type.Type but is %s" % repr(t))
            
        self._val = v
        self._setType(t)
    
    ### Overridden from Operand.
    
    def constant(self):
        return None
    
    def llvmLoad(self, cg):
        return self._coerce(cg, self._val)
        
    def llvmStore(self, cg, val):
        cg.llvmAssign(val, self._val)
    
    def __str__(self):
        return "<LLVM Operand of type %s>" % self.type()
    
def coerceTypes(op1, op2):
    """Determines a joint type into which two operands can be coerced. 
    
    Returns: ~~Type - The type into which both *op1* and *op2* can be coerced,
    or None if no such type could be found.
    """
    if op1.canCoerceTo(op2.type()):
        return op2.type()
    
    if op2.canCoerceTo(op1.type()):
        return op1.type()
    
    return None
