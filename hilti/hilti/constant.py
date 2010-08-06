# $Id$
# 
# A constant.

import location
import node
import type

class Constant(node.Node):
    """Represents a constant value with its type.
    
    value: any - The constant's value in accordance with *type*
    ty: ~~Type -- The constant's type.
    location: ~~Location - A location to be associated with the constant. 
    
    Note: The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    def __init__(self, value, ty, location=None):
        self._value = value
        self._type = ty
        self._location = location
        assert isinstance(self.type(), type.Constable)
        
    def value(self):
        """Returns the value of the constant.
        
        Returns: any - Value of the constant.
        """
        return self._value
    
    def type(self):
        """Returns the type of the constant.
        
        Returns: ~~Type - Type of the constant.
        """
        return self._type

    def setType(self, type):
        """Sets the type of the constant.
        
        type: ~~Type - The type to set.
        """
        self._type = type

    def location(self):
        """Returns the location associated with the constant.
        
        Returns: ~~Location - The location. 
        """
        return self._location

    def llvm(self, cg):
        """Compiles the constants into its LLVM equivalent.
        
        Returns: llvm.core.Constant - The LLVM constant. 
        """
        return self._type.llvmConstant(cg, self)
    
    def __str__(self):
        return " %s %s" % (self._type, self._value)

    # Overridden from Node.
    def _validate(self, vld):
        if not isinstance(self.type(), type.Constable):
            vld.error(self, "cannot create constants of type %s" % self.type())
            return False
        
        self._type.validate(vld)
        self._type.validateConstant(vld, self)

    def _resolve(self, resolver):
        self._type = self._type.resolve(resolver)
        self._type.resolveConstant(resolver, self)
       
    def output(self, printer):
        self._type.outputConstant(printer, self)
    
    # Visitor support.
    def visitorSubType(self):
        return self._type

Void = Constant("void", type.Void())
"""A predefined ~~Constant representing a *void* value."""
    
