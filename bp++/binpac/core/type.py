# $Id$

import binpac.support.util as util

class Type(object):
    """Base class for all data types provided by the BinPAC++ language.  

    prod: bool - True if the type can be used inside a ~~Unit declaration; if
    so, the method ~~production must be overridden.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, prod=False, location=None):
        self._prod = prod
        self._location = location
    
    def name(self):
        """Returns a short name for the type. The name can be used
        in message to the user. 
        
        This function must be overriden by derived classes.
        """
        util.internal_error("Type.name() not overidden for %s" % self.__class__)

    def toCode(self):
        """Returns the type's full declaration. 
        
        This function must be overriden by derived classes. For all
        types that a user can directly used, the returned name must
        be parseable by the BinPAC++ parser and fully describe the
        type, including all attributes.
        
        Returns: string - The full type declaration. 
        """
        util.internal_error("Type.toCode() not overidden for %s" % self.__class__)

    def hasProduction(self):
        """Returns whether the type can used within a unit declaration.
        
        Returns: bool - True if it can used inside a ~~Unit.
        """
        return self._prod
        
    def validate(self, vld):
        """Validates the semantic correctness of the type during code
        generation. 
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that also
        need to be checked, the method needs to do that recursively.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        pass
        
    def hiltiType(self, cg, tag):
        """Returns the corresponding HILTI type. 
        
        Must be overridden by derived classes for type that can be used by an
        HILTI program. The methods can use *codegen* to add type declaratins
        (or anything else) to the HILTI module.
        
        codegen: ~~CodeGen - The current code generator. 
        
        tag: string - A string that can be used to build meaningful ID names
        when generating code via the code generator. There is not further
        interpretation of the tag; it's mainly to generate more readable HILTI
        code. For example, a *tag* might be the name of an BinPAC ID
        accociated with the type.
        
        Returns: hilti.core.type.Type - The HILTI type.
        """
        util.internal_error("Type.hiltiType() not overidden for %s" % self.__class__)

    def production(self):
        """Returns a production for parsing instaces of this type.

        Must be overridden by derived classes if *prod==True* was passed to
        the ctor.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)
        
    def location(self):
        """Returns the location associated with the type.
        
        Returns: ~~Location - The location. 
        """
        return self._location
    
    def __str__(self):
        return self.name()

class TypeDecl(Type):
    """Type for type declarations.  
    
    t: ~~Type - The declared type.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, t, location=None):
        super(TypeDecl, self).__init__(location=location)
        self._decl = t
        
    def declType(self):
        """Returns the type declared.
        
        Returns: ~~Type - The type.
        """
        return self._decl

    def name(self):
        return "<type-decl>"
        
class Integer(Type):
    """Base type for integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(Integer, self).__init__(location=location)
        assert(width > 0 and width <= 64)
        self._width = width

    def width(self):
        """Returns the bit-width of the type's integers.
        
        Returns: int - The number of bits available to represent integers of
        this type. 
        """
        return self._width

    def setWidth(self, width):
        """Sets the bit-width of the type's integers.
        
        width: int - The new bit-width. 
        """
        self._width = width

    def validate(self, vld):
        # Overridden from Type.
        if self._width < 1 or self._width > 64:
            vld.error(self, "integer width out of range")


# Trigger importing the other types into our namespace.

def pac(ty):
    """Class decorator to add a type class that is defined in some other
    module to the namespace of *binpac.core.type*.
    
    ty: class - A class derived from ~~Type.
    """
    import binpac.core
    binpac.core._types[ty.__name__] = ty
    globals()[ty.__name__] = ty

    return ty

# No longer necessary?
#for (name, ty) in binpac.core._types.items():
#    globals()[name] = ty

