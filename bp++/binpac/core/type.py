# $Id$

import binpac.support.util as util

import hilti.core.constant

class Type(object):
    """Base class for all data types provided by the BinPAC++ language.  

    params: list of ~~Expr - The list of type parameters; their number and
    type must correspond to what ~~supportedParameters returns (except for
    parameters with defaults, which may be missing). 
    
    location: ~~Location - A location object describing the point of definition.
    
    Throws: ~~ParameterError - If *params* don't match what
    ~~supportedParameters specifies. 
    """

    _type_name = "<type_name not set>" # Set by the pac() the decorator.
    
    class ParameterError(Exception):
        """Signals a type parameter error."""
        pass
    
    def __init__(self, params=[], location=None):
        self._location = location
        self._params = []
        
        all = self.supportedParameters()
        
        if len(params) > len(all):
            raise ParameterError, "too many parameters for type"
        
        for i in range(len(all)):
            ty = all[i][0]
            default = all[i][1]
            
            if i >= len(params):
                if default:
                    param = default
                else:
                    raise ParameterError, "not enough parameters for type"
            else:
                param = params[i]
                
                if param.type() != ty:
                    raise ParameterError, "type parameter must be of type %s" % ty
                
            if not param.isConst():
                raise ParameterError, "type parameter must be a constant"
                
            self._params += [param]
        
    def parameters(self):
        """Returns the types parameters.
        
        Returns: list of ~~Expr - Each entry corresponds to the corresponding
        index in what ~~supportedParameters returns; missing parameters are
        replaces with their defaults"""
        return self._params
        
    def location(self):
        """Returns the location associated with the type.
        
        Returns: ~~Location - The location. 
        """
        return self._location

    def __ne__(self, other):
        return not self.__eq__(other)
    
    def __str__(self):
        return self.name()

    @classmethod
    def typeName(cls):
        """Returns a short, descriptive name for instances of the type. These
        names can be used in messages to the user. 
        
        Note: The returned name is what's defined by the ~~pac decorator. 
        """
        return cls._type_name
    
    ### Methods for derived classes to override.    
    
    def name(self):
        """Returns a short name for the type. The name can be used
        in messages to the user. 
        
        This function can be overriden by derived classes. The default
        implementation returns just the same as ~~typeName().
        """
        return self.__class__.typeName()

    def supportedParameters(self):
        """Returns the type parameters this type supports.
        
        This function can be overriden by derived classes. The default
        implementation returns the empty list, i.e., no type parameters.
        
        Returns: list of pairs (~~Type, ~~Expr) - The supported type paramters
        in the order as they must be given; the first element defines the type
        of the parameter, the second a default expression which will be used if the
        parameter is not given.
        """ 
        return []
        
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
        
        Must be overridden by derived classes for types that can be used by an
        HILTI program. The methods can use *codegen* to add type declarations
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

    def hiltiConstant(self, const, codegen, builder):
        """Returns the HILTI constant for a constant of this type.
        
        Can be overridden by derived classes for types. The default
        implementation turns the constant into a HILTI constant of type
        ~~hiltiType(), keeping the same value as *const* has. This works for
        all types for which the values are represented in the same interal way
        in BinPAC and HILTI.
        
        *const*: ~~Constant - The constant to convert to HILTI.
        
        Returns: hilti.core.constant.Constant - The HILTI constant.
        """
        hlt = const.type().hiltiType(codegen, builder)
        return hilti.core.constant.Constant(const.value(), hlt)
        
    def toCode(self):
        """Returns the type's full declaration as readable string. 
        
        This function must be overriden by derived classes. For all
        types that a user can directly used, the returned string must
        be parseable by the BinPAC++ parser and fully describe the
        type, including all attributes.
        
        Returns: string - The full type declaration. 
        """
        util.internal_error("Type.toCode() not overidden for %s" % self.__class__)

    def __eq__(self, other):
        """Compare two types for compatibility. If the comparision yields
        *True*, the two types are considered equivalent. This operator is
        BinPAC's main way of ensuring type-safety. 
        
        Can be overridden by derived classed. The default implementation
        returns ``NotImplemented`` if *other* is an instance of a different
        class than ``self``. If both are of the same class, it returns *True*
        if all ~parameters match, and *False* otherwise. 
        """
        if not isinstance(other, self.__class__):
            return NotImplemented

        if len(self.parameters()) != len(other.parameters()):
            return False
        
        for (p1, p2) in zip(self.parameters(), other.parameters()):
            if p1 != p2:
                return False
            
        return False
        
class ParseableType(Type):
    """Type that can be parsed from an input stream. Only these types can be
    used inside a ~~Unit.
    
    attrs: list of (name, value) pairs - The attributes for this type. They
    will be checked against what ~~supportedAttributes returns; if they don't
    match, an ``AttributeError`` exception is thrown. *name* is a string
    (excluding the ampersand), and *expr* is an ~~Expr.
    """
    def __init__(self, params=[], attrs=[], location=None):
        super(ParseableType, self).__init__(params=params, location=location)
        self._attrs = {}
        
        for (name, expr) in attrs:
            self.addAttribute(name, expr)

    def hasAttribute(self, name):
        """Returns whether an attribute has been defined. If an attribute has
        a default expression, it is always returned as being defined.
        
        name: string - The name of the attribute. The name must be returned by
        ~~supportedAttributes.
        
        Returns: bool - True if the attribute is defined.
        """
        if name in self._attrs:
            return True
        
        try:
            all = self.supportedAttributes()
            (ty, const, default) = all[name]
        except KeyError:
            util.internal_error("attribute %s not valid for type %s" % (name, self))
            
        if default:
            return True
        
        return False
        
    def attributeExpr(self, name):
        """Returns the expression associated with an attribute. If the
        attribute is not defined, or if the attribute is defined but does not
        have an expression, *None* is returned. If the value is not defined
        but has a default expression, the default is returned.

        name: string - The name of the attribute. The name must be returned by
        ~~supportedAttributes.
        
        Returns: ~~Expr or None - The expression associated with the
        attribute, or None as described above.
        """
        try:
            all = self.supportedAttributes()
            (ty, const, default) = all[name]
        except KeyError:
            util.internal_error("attribute %s not valid for type %s" % (name, self))
            
        if name in self._attrs:
            return self._attrs[name]
        
        return default

    class AttributeMismatch(Exception):
        pass
    
    def addAttribute(self, name, expr):
        """Adds an attribute to the type.
        
        name: string - The name of the attribute, which must be among those
        reported by ~~supportedAttributes.
        
        expr: ~~Expr - a constant ~~Expression giving the attribute's
        expression, or None if the attributes doesn't have an expression. Presence
        and type of *expr* must correspond to what ~~supportedAttributes
        specifies.
        
        Throws: ~~AttributeMismatch - If the attribute does not match with what
        ~supportedAttributes specifies.
        """
        if expr and not expr.isConst():
            raise ParseableType.AttributeMismatch, "attribute expr must be constant"
 
        try:
            all = self.supportedAttributes()
            (ty, const, default) = all[name]
            
            if ty and not expr:
                raise ParseableType.AttributeMismatch, "attribute must have expression of type %s" % ty
            
            if expr and not ty:
                raise ParseableType.AttributeMismatch, "attribute cannot have an expression"
            
            if const and not expr.isConst():
                raise ParseableType.AttributeMismatch, "attribute's expression must be a constant"

            if not expr.canCastTo(ty):
                raise ParseableType.AttributeMismatch, "attribute's expression must be of type %s, but is %s" % (ty, expr.type())

            self._attrs[name] = expr.simplify()
            
        except KeyError:
            raise AttributeMismatch, "unknown type attribute &%s" % name

    ### Methods for derived classes to override.    
    
    def supportedAttributes(self):
        """Returns the attributes this type supports.
        
        Returns: dictionary *name* -> (*type*, *constant*, *default*) - *name*
        is a string with the attribute's name (excluding the leading
        ampersand); *type* is a ~~Type defining the type the attribute's
        expression must have, or None if the attribute doesn't take a
        expression; *constant* is a boolean indicating whether the attribute's
        expression must be constant; and *default* is a ~~Constant expression
        with a default expression to be used if the attributes is not
        explicitly specified, or None if the attribute is unset by default.

        This method can be overridden by derived classes. The default
        implementation returns an empty dictionary, i.e., no supported attributes.
        """
        return {}
        
    def production(self):
        """Returns a production for parsing instances of this type.

        The method must be overridden by derived classes.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)
        
    def generateParser(self, codegen, builder, cur, dst, skipping):
        """Generates code for parsing an instace of the type. 
        
        The method must be overridden by derived classes.
        
        codegen: CodeGen - The current code generator.. 
        
        builder: hilti.core.builder.Builder - The HILTI builder to use. 
        
        dst: hilti.core.instruction.Operand - The operand in which to store
        the parsed value. The operand will have the type returned by
        ~~hiltiType. 
        
        cur: hilti.core.instruction.Operand - A bytes iterator with the
        position where to start parsing. 
        
        skipping: boolean - True if the parsed value will actually never be
        used. If so, the function is free to skip fully parsing it, as well as
        storing anything in *dst*. It however must still return the advanced
        iterator.
        
        Returns: tuple (hilti.core.instruction.Operand,
        hilti.core.builder.Builder) - The first element is a a byte iterator
        containing the advanced parsing position, pointing to the first byte
        *not* consumed anymore; the second element is the builder to use for
        subsequent code.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)
    
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

# Trigger importing the other types into our namespace.

def pac(name):
    """Class decorator to add a type class that is defined in some other
    module to the namespace of *binpac.core.type*.

    name: a short, descriptive name for the type that be used in messages to
    the users to describe instances of the type. 
    
    ty: class - A class derived from ~~Type.
    """
    
    def _pac(ty):
        import binpac.core
        binpac.core._types[ty.__name__] = ty
        globals()[ty.__name__] = ty
        ty._type_name = name
        return ty

    return _pac

