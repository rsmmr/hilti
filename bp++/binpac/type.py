# $Id$

builtin_id = id

import copy

import scope
import id
import binpac.util as util

import hilti.constant

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

    _counter = 1
    
    class ParameterError(Exception):
        """Signals a type parameter error."""
        pass
    
    def __init__(self, params=[], location=None):
        super(Type, self).__init__()
        
        self._location = location
        self._params = []
        self._name = "type_%d" % Type._counter
        
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

    def name(self):
        """Returns the type object's name. This can be used, e.g., to track
        assigment of types to ~~IDs. A unique default name is chosen
        automatically. 
        
        Returns: string - The name.
        """
        return self._name
    
    def setName(self, name):
        """Set the type object's name. This can be used, e.g., to track
        assigment of types to ~~IDs. 
        
        name: string - The name.
        """
        self._name = name
            
    def parameters(self):
        """Returns the types parameters.
        
        Returns: list of ~~Expr - Each entry corresponds to the corresponding
        index in what ~~supportedParameters returns; missing parameters are
        replaced with their defaults"""
        return self._params
        
    def location(self):
        """Returns the location associated with the type.
        
        Returns: ~~Location - The location. 
        """
        return self._location

    def setLocation(self, location):
        """Sets the location associated with the type.
        
        location: ~~Location - The location. 
        """
        self._location = location

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

    def canCastTo(self, dsttype):
        """Returns whether a this type can be coerced to a destination type.
        
        Can be overridden by derived classes. The default implementation
        returns True iff *dstype* is equals the curren type.
        
        dsttype: ~~Type - The target type.
        
        Returns: bool - Whether the coercion is ok.
        """
        return self == dsttype
    
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

    def resolve(self, resolver):
        """Resolves any previously unknown types that might be
        referenced by this type. (Think: forward references). For
        these, the not-yet-known types will have been created as
        ~~Unknown; later, when the type should be known, this method
        will be called to then lookup the real type. If an error is
        encountered (e.g., we still don't the identifier), an error
        message will be reported via the *resolver*. 
        
        resolver: ~~Resolver - The current resolver to use. 
        
        Return: ~~Type - Returns a type equivalent to *self* with
        all unknowns resolved; that may either be *self* itself, or
        a newly instantiated type; the caller must then use the
        returned type instead of *self* afterwards.
        
        Note: ~~Type is not derived from ~~Node and thus we are not
        overriding ~~Node.resolve here, even though it might
        initially look like that. This method does work similar but
        it returns a value, which ~~Node.resolve does not. 
        """
        return self
    
    def validate(self, vld):
        """Validates the semantic correctness of the type.
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that also
        need to be checked, the method needs to do that recursively.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        pass
    
    def validateConst(self, vld, const):
        """Validates the semantic correctness of a constant of the
        type.
        
        Must be overidden by derived classes if constants with their
        type can be created.
        
        vld: ~~Validator - The validator triggering the validation.
        const: ~~Constant - The constant, which will have the type's type.
        """
        util.internal_error("Type.validateConst() not overidden for %s" % self.__class__)
        
    def validateCtor(self, vld, value):
        """Validates the semantic correctness of a ctor value of the type.
        
        Must be overidden by derived classes if ctors with their type can be
        created.
        
        vld: ~~Validator - The validator triggering the validation.
        value: any - The value in type-specific type. 
        """
        util.internal_error("Type.validateCtor() not overidden for %s" % self.__class__)
        
    def hiltiType(self, cg):
        """Returns the corresponding HILTI type. 
        
        Must be overridden by derived classes for types that can be used by an
        HILTI program. The methods can use *cg* to add type declarations
        (or anything else) to the HILTI module.
        
        cg: ~~CodeGen - The current code generator. 
        
        Returns: hilti.type.Type - The HILTI type.
        """
        util.internal_error("Type.hiltiType() not overidden for %s" % self.__class__)

    def hiltiConstant(self, cg, const):
        """Returns the HILTI constant for a constant of this type.
        
        Can be overridden by derived classes for types. The default
        implementation turns the constant into a HILTI constant of type
        ~~hiltiType(), keeping the same value as *const* has. This works for
        all types for which the values are represented in the same interal way
        in BinPAC and HILTI.
        
        cg: ~~CodeGen - The current code generator. 
        *const*: ~~Constant - The constant to convert to HILTI.
        
        Returns: hilti.constant.Constant - The HILTI constant.
        """
        hlt = const.type().hiltiType(cg)
        return hilti.constant.Constant(const.value(), hlt)

    def hiltiCtor(self, cg, value):
        """Returns a HILTI ctor operand for constructing an element of this type.
        
        Must be overridden by derived classes for types that provide ctor
        expressions.
        
        cg: ~~CodeGen - The current code generator. 
        *cal*: any - The value of a type-specififc type.
        
        Returns: hilti.operand.Ctor - The HILTI ctor.
        """
        util.internal_error("Type.hiltiCtor() not overidden for %s" % self.__class__)
    
    def hiltiDefault(self, cg, must_have=True):
        """Returns the default value to initialize HILTI variables of this
        type with.

        cg: ~~CodeGen - The current code generator. 
        
        must_have: bool - If False, it is an option to leave return
        None of there is no particular default. This is for example
        useful for structs where we can leave fields unset. If 
        True, the funtion must return a value. 
        
        Can be overridden by derived classes if the default value set by
        HILTI for variables is not the desired one. 
        
        Returns: hilti.operand.Operand - The default value, of None
        if the HILTI default is correct or *must_have* is False and
        no default value is picked. 
        """
        return None
    
    def pac(self, printer):
        """Converts the type into parseable BinPAC++ code.

        Must be overidden by derived classes.
        
        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Type.pac() not overidden for %s" % self.__class__)

    def pacConstant(self, printer, value):
        """Converts the a constant of the type into its BinPAC++ representation.

        Must be overidden by derived classes if constants with their type can
        be created.
        
        value: ~~Constant - The constant of the type.
        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Type.pacConstant() not overidden for %s" % self.__class__)
        
    def pacCtor(self, printer, value):
        """Converts a ctor of the type into its BinPAC++ representation.

        Must be overidden by derived classes if ctors with their type can be
        created.
        
        value: any - The value of the ctor in a type-specific type.
        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Type.pacCtor() not overidden for %s" % self.__class__)

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
            
        return True
        
class ParseableType(Type):
    """Type that can be parsed from an input stream. Only these types can be
    used inside a ~~Unit.
    
    params: list of any - The type's parameters.
    """
    def __init__(self, params=[], location=None):
        super(ParseableType, self).__init__(params=params, location=location)
        self._attrs = {}
        self._pgen = None

    def parserGen(self):
        """Returns the parser generator used for this type. Only valid once
        the grammar for this unit has been compiled.
        
        Returns: ~~ParserGen - The parser generator that compiled this unit.
        """
        return self._pgen
        
    def hasAttribute(self, name):
        """Returns whether an attribute has been defined. If an attribute has
        a default expression, it is always returned as being defined.
        
        name: string - The name of the attribute, without the leading
        ampersand. The name must be returned by ~~supportedAttributes.
        
        Returns: bool - True if the attribute is defined.
        """
        if name in self._attrs:
            return True
        
        try:
            all = self.supportedAttributes()
            (ty, const, default) = all[name]
        except KeyError:
            return None
            
        if default:
            return True
        
        return False
        
    def attributeExpr(self, name):
        """Returns the expression associated with an attribute. If the
        attribute is not defined, or if the attribute is defined but does not
        have an expression, *None* is returned. If the value is not defined
        but has a default expression, the default is returned.

        name: string - The name of the attribute, without the leading
        ampersand. The name must be returned by ~~supportedAttributes.
        
        Returns: ~~Expr or None - The expression associated with the
        attribute, or None as described above.
        """
        try:
            all = self.supportedAttributes()
            (ty, const, default) = all[name]
        except KeyError:
            return None
            
        if name in self._attrs:
            return self._attrs[name]
        
        return default

    class AttributeMismatch(Exception):
        pass
    
    def addAttribute(self, name, expr):
        """Adds an attribute to the type.
        
        name: string - The name of the attribute without the leading
        ampersand, which must be among those reported by
        ~~supportedAttributes.
        
        expr: ~~Expr - a constant ~~Expression giving the attribute's
        expression, or None if the attributes doesn't have an expression. Presence
        and type of *expr* must correspond to what ~~supportedAttributes
        specifies.
        """
        self._attrs[name] = expr.simplify() if expr else None
        
    def validate(self, vld):
        for (name, expr) in self._attrs.values():
            
            all = self.supportedAttributes()
            if name not in all:
                vld.error(self, "unknown type attribute &%s" % name)
                continue
            
            (ty, init, default) = all[name]
            
            if ty and not expr:
                vld.error(self, "attribute must have expression of type %s" % ty)
            
            if expr and not ty:
                vld.error(self, "attribute cannot have an expression")

            if init and not expr.isInit():
                vld.error(self, "attribute's expression must be suitable for initializing a value")

            if not expr.canCastTo(ty) and not isinstance(ty, Any):
                vld.error(self, "attribute's expression must be of type %s, but is %s" % (ty, expr.type()))
            
            if expr:
                expr.validate(vld)
        
    def resolve(self, resolver):
        for expr in self._attrs.values():
            if expr:
                expr.resolve(resolver)
                
        return self
        
    ### Methods for derived classes to override.    

    def parsedType(self):
        """Returns the type of values parsed by this type. This type will
        become the type of the corresponding ~~Unit field. While usually the
        parsed type is the same as the type itself (and that's what the
        default implementation returns), it does not need to. The ~~RegExp
        type returns ~~Bytes for example.
        
        This method can be overridden by derived classes. The default
        implementation returns just *self*. 
        
        Returns: ~~Type - The type for parsed fields.
        """
        return self
    
    def supportedAttributes(self):
        """Returns the attributes this type supports.
        
        Returns: dictionary *name* -> (*type*, *init*, *default*) -
        *name* is a string with the attribute's name (excluding the
        leading ampersand); *type* is a ~~Type defining the type the
        attribute's expression must have, or None if the attribute
        doesn't take a expression; *init* is a boolean indicating
        whether the attribute's expression must suitable for
        intializing a HILTI variable; and *default* is a ~~Constant
        expression with a default expression to be used if the
        attributes is not explicitly specified, or None if the
        attribute is unset by default.

        This method can be overridden by derived classes. The
        default implementation returns an empty dictionary, i.e., no
        supported attributes. 
        
        Note if you want to support the ``&default`` attribute, you need to
        overwrite this method and add an entry ``{ "default": (self, True,
        None) }``. You don't need to do anything else than that though.
        """
        return {}

    def validateInUnit(self, vld):
        """Validates the semantic correctness of the type when used inside a
        unit definition.
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that
        also need to be checked, the method needs to do that recursively.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        pass
    
    def initParser(self, field):
        """Hook into parser initialization. The method will be called at the
        time a ~~UnitField is created that has this type as its
        ~~UnitField.type. 
        
        This method can be overridden by derived classes if they need to
        perform some field-specific initialization already before *production*
        is called. An example is setting the field's ~~FieldControlHook.  The
        default implementation does nothing. 
        
        field: ~~UnitField - The newly instantiated unit field. 
        """
        pass
    
    def production(self, field):
        """Returns a production for parsing instances of this type.
        
        field: ~~Field - The unit field of type *self* that is to be parsed. 

        The method must be overridden by derived classes.
        
        Note: It is guarenteed that ~~initParser will have been called with
        the same *field* before ~~production.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)
        
    def generateParser(self, cg, cur, dst, skipping):
        """Generate code for parsing an instance of the type. 
        
        The method must be overridden by derived classes which may be used as
        the type of ~~Variable production.
        
        cg: ~~CodeGen - The current code generator. 
        
        dst: hilti.operand.Operand - The operand in which to store
        the parsed value. The operand will have the type returned by
        ~~hiltiType. 
        
        cur: hilti.operand.Operand - A bytes iterator with the
        position where to start parsing. 
        
        skipping: boolean - True if the parsed value will actually never be
        used. If so, the function is free to skip fully parsing it, as well as
        storing anything in *dst*. It however must still return the advanced
        iterator.
        
        Returns: ~~hilti.operand.Operand - A a byte iterator
        containing the advanced parsing position, pointing to the first byte
        *not* consumed anymore.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)

class Unknown(ParseableType):
    """Type for an identifier that has not yet been resolved. This is used
    initially for forward references, and then later replaced the actual type.
    
    idname: string - The name of the identifier that needs to be resolved.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, idname, location=None):
        super(Unknown, self).__init__(location=location)
        self._id = idname
        
    def idName(self):
        """Returns the name of the ID that needs to be resolved."""
        return self._id

    # Overidden from Type.
    
    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        i = resolver.scope().lookupID(self._id)
        
        if not i:
            resolver.error(self, "undefined type id %s" % self._id)
            return self
        
        if not isinstance(i, id.Type):
            resolver.error(self, "identifier %s does not refer to a type" % self._id)
            return self 

        t = copy.deepcopy(i.type())
        t._attrs = self._attrs
        return t

    _type_name = "<unknown type>" 
    
class Identifier(Type):
    """Type for an unbound identifier.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Identifier, self).__init__(location=location)

    def validateConst(self, vld, const):
        if not isinstance(const.value(), str):
            vld.error(const, "identifier: constant of wrong internal type")
            
    def pac(self, printer):
        printer.output("<type.Identifier>") # Should not get here.
        
    def pacConstant(self, printer, value):
        printer.output(value.value())
        
    def hiltiType(self, cg):
        return hilti.type.String()
    
class MetaType(Type):
    """Type for types. 
    
    t: ~~Type - The type referenced. 
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, t, location=None):
        super(MetaType, self).__init__(location=location)
        self._type = t

    def type(self):
        """Returns the referenced type.
        
        Returns: Type - The type.
        """
        return self._type

    # Overidden from Type.
    
    def resolve(self, resolver):
        self._type = self._type.resolve(resolver)
        return self
        
    def validate(self, vld):
        self._type.validate(vld)
    
    def pac(self, printer):
        self._type.output(printer)
        
    def hiltiType(self, cg):
        util.internal_error("hiltiType() not defined for MetaType")
        
class Any(Type):
    """Type indicating a match with any other type.
    """
    pass
        
# Additional traits types may have.
#class Derefable(object):
#    """A deref'able type can appear as the LHS in attribute expressions of the
#    form ``foo.bar``. It has a scope of valid attributes that is initially
#    empty. 
#    """
#    def __init__(self):
#        super(Derefable, self).__init__()
#        self._scope = scope.Scope(None, None)
    
# Trigger importing the other types into our namespace.

def pac(name):
    """Class decorator to add a type class that is defined in some other
    module to the namespace of *binpac.type*.

    name: a short, descriptive name for the type that be used in messages to
    the users to describe instances of the type. 
    
    ty: class - A class derived from ~~Type.
    """
    
    def _pac(ty):
        import binpac
        binpac._types[ty.__name__] = ty
        globals()[ty.__name__] = ty
        ty._type_name = name
        return ty

    return _pac

