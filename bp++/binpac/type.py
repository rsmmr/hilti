# $Id$

builtin_id = id

import copy

import node
import scope
import id
import type
import binpac.util as util
import binpac.pgen as pgen

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
    
    def __init__(self, params=None, location=None):
        super(Type, self).__init__()
        
        if not params:
            params = []
        
        self._location = location
        self._params = []
        self._name = "type_%d" % Type._counter
        self._resolved = False
        self._attrs = {}
        
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
                
            if not param.isInit():
                raise ParameterError, "type parameter must be an initialization value"
                
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

    def namespace(self):
        """Returns the namespace in which the type was defined, if specified.
        
        Returns: string - The namespace, or None.
        """
        return self._namespace
    
    def setNamespace(self, namespace):
        """Sets the namespace the type is defined in.
        
        namespace: string - The namespace.
        """
        self._namespace = namespace
        
    def parameters(self):
        """Returns the types parameters.
        
        Returns: list of ~~Expr - Each entry corresponds to the corresponding
        index in what ~~supportedParameters returns; missing parameters are
        replaced with their defaults"""
        return self._params

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
        self._attrs[name] = expr

    def copyAttributesFrom(self, other):
        """XXXX"""
        
        if builtin_id(self._attrs) == builtin_id(other._attrs):
            return
        
        # self._attrs = {}
        for (key, val) in other._attrs.items():
            self._attrs[key] = val
    
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

    def resolve(self, resolver):
        """Resolves any previously unknown types that might be
        referenced by this type. (Think: forward references). For
        these, the not-yet-known types will have been created as
        ~~Unknown; later, when the type should be known, this method
        will be called to then lookup the real type. If an error is
        encountered (e.g., we still don't the identifier), an error
        message will be reported via the *resolver*. 
        
        Derived classes should *not* call their parent's implementation.
        
        resolver: ~~Resolver - The current resolver to use. 
        
        Return: ~~Type - Returns a type equivalent to *self* with
        all unknowns resolved; that may either be *self* itself, or
        a newly instantiated type; the caller must then use the
        returned type instead of *self* afterwards.
        
        Note: ~~Type is not derived from ~~Node and thus we are not
        overriding ~~Node.resolve here, even though it might
        initially look like that. This method does work similar but
        it returns a value, which ~~Node.resolve does not. If you want to
        resolve attributes of the class (rather than an instance of the class
        itself), overridde ~~doResolve instead.
        """
        if self._resolved:
            return self
        
        self._resolved = True
        self.doResolve(resolver)
        return self
        
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
    
    # @node.check_recursion # Expensinve

    def validate(self, vld):
        Type.validate(self, vld)
        
    def doResolve(self, resolver):
        """XXXX"""
        for expr in self._attrs.values():
            if expr:
                expr.resolve(resolver)
    
    def validate(self, vld):
        """Validates the semantic correctness of the type.
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that also
        need to be checked, the method needs to do that recursively.

        Derived classes should call their parent's implementation.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        for (name, expr) in self._attrs.items():
            
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

            if ty and not expr.canCoerceTo(ty) and not isinstance(ty, Any):
                vld.error(self, "attribute's expression must be of type %s, but is %s" % (ty, expr.type()))
            
            if expr:
                expr.validate(vld)
        
    def validateCtor(self, vld, value):
        """Validates the semantic correctness of a ctor value of the type.
        
        Must be overidden by derived classes if ctors with their type can be
        created.
        
        vld: ~~Validator - The validator triggering the validation.
        value: any - The value in type-specific type. 
        """
        util.internal_error("Type.validateCtor() not overidden for %s" % self.__class__)

    def supportedAttributes(self):
        """Returns the attributes this type supports.
        
        Returns: dictionary *name* -> (*type*, *init*, *default*) -
        *name* is a string with the attribute's name (excluding the
        leading ampersand); *type* is a ~~Type defining the type the
        attribute's expression must have, or None if the attribute
        doesn't take a expression; *init* is a boolean indicating
        whether the attribute's expression must suitable for
        intializing a HILTI variable; and *default* is a ~~Ctor
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
        
    def hiltiType(self, cg):
        """Returns the corresponding HILTI type. 
        
        Must be overridden by derived classes for types that can be used by an
        HILTI program. The methods can use *cg* to add type declarations
        (or anything else) to the HILTI module.
        
        cg: ~~CodeGen - The current code generator. 
        
        Returns: hilti.type.Type - The HILTI type.
        """
        util.internal_error("Type.hiltiType() not overidden for %s" % self.__class__)

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
    def __init__(self, params=None, location=None):
        super(ParseableType, self).__init__(params=params, location=location)
        self._pgen = None

    def parserGen(self):
        """Returns the parser generator used for this type. Only valid once
        the grammar for this unit has been compiled.
        
        Returns: ~~ParserGen - The parser generator that compiled this unit.
        """
        return self._pgen
        
    def generateUnpack(self, cg, op1, op2, op3=None):
        """Generates a HILTI ``unpack`` instruction wrapped in error handling
        code. The error handling code will do "the right thing" when either a
        parsing error occurs or insufficient input is found. In the latter
        case, it will execute the code generated by
        ~~generateInsufficientInputHandler.
        
        cg: ~~CodeGen - The current code generator.
        
        op1, op2, op3: ~~hilti.Operand - Same as with the regular ``unpack``
        instruction. 
        
        Returns: ~~hilti.Operand - The result of the unpack.
        """
        
        fbuilder = cg.functionBuilder()
        
        parse = fbuilder.newBuilder("parse")
        cg.builder().jump(parse.labelOp())
        
        try_ = fbuilder.newBuilder(None, add=False)
        error = fbuilder.newBuilder(None, add=False)
        insufficient = fbuilder.newBuilder(None, add=False)
        cont = fbuilder.newBuilder("cont")
        
        bytesit = hilti.type.IteratorBytes(hilti.type.Bytes())
        resultt = hilti.type.Tuple([self.hiltiType(cg), bytesit])
        result = fbuilder.addLocal("__unpacked", resultt)
        try_.unpack(result, op1, op2, op3)
        
        error.makeRaiseException("BinPAC::ParseError", error.constOp("unpack failed"))
        
        cg.setBuilder(insufficient)
        iter = fbuilder.addTmp("__iter", bytesit)
        cg.builder().tuple_index(iter, op1, fbuilder.constOp(0))
        self.generateInsufficientInputHandler(cg, iter)
        cg.builder().jump(parse.labelOp())
        
        catch1 = (None, [error.block()])
        catch2 = (fbuilder.idOp("Hilti::WouldBlock").type(), [insufficient.block()])

        ins = hilti.instructions.trycatch.TryCatch([try_.block()], [catch1, catch2])
        parse.block().addInstruction(ins)
        parse.jump(cont.labelOp())
        
        cg.setBuilder(cont)
        return result

    def generateInsufficientInputHandler(self, cg, iter):
        """Function generating code for handling insufficient input. If
        parsing a instance finds insufficient input, it must execute the code
        generated by this method and then retry the same operation afterwards.
        The generated code handles both the case where more input may become
        available later (in which case it will return and let the caller try
        again), and the case where it may not (in which case it will throw an
        exception and never return).
        
        cg: ~~CodeGen - The current code generator.
        
        iter: ~~hilti.Operand - A HITLI operand with the ``iterator<bytes>``
        from which is being parsed. 
        """
        
        # If our flags indicate that we won't be getting more input, we throw
        # a parse error. Otherwise, we yield and return. 
        fbuilder = cg.functionBuilder()
        
        error = fbuilder.newBuilder("error")
        error.makeDebugMsg("binpac-verbose", "parse error, insufficient input")
        error.makeRaiseException("BinPAC::ParseError", error.constOp("insufficient input"))
        
        suspend = fbuilder.newBuilder("suspend")
        suspend.makeDebugMsg("binpac-verbose", "out of input, yielding ...")
        suspend.yield_()

        cont = fbuilder.addTmp("__cont", hilti.type.Bool())
        
        builder = cg.builder()
        builder.bytes_is_frozen(cont, iter)
        builder.if_else(cont, error.labelOp(), suspend.labelOp())

        cg.setBuilder(suspend)
        
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

    def fieldType(self):
        """Returns the final type of the field parsed with this type. This
        will usually be the same as ~~parsedType, but may differ if additional
        post-filter function are applied. 
        
        This method can be overridden by derived classes. The default
        implementation returns just ~~parsedType.
        
        Returns: ~~Type - The type for parsed fields.
        """
        return self.parsedType()

    def hiltiUnitDefault(self, cg):
        """Returns the default value to initialize attributes of this type
        with in a ~~Unit.

        This method can be overridden by derived classes. The default
        implementation returns None. 
        
        Returns: ~~hilti.operand.Ctor or None - The default value, or None if
        per default the attributes are left to be unset. 
        """
    
    def validateInUnit(self, field, vld):
        """Validates the semantic correctness of the type when used inside a
        unit definition.
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that
        also need to be checked, the method needs to do that recursively.
        
        field: ~~unit.Field - The unit type the type is part of. 
        
        vld: ~~Validator - The validator triggering the validation.
        """
        pass
    
    def initParser(self, field):
        """Hook into parser initialization. The method will be called at the
        time a ~~UnitField has resolved all it unknown symbols but not
        performed anything else yet. 
        
        This method can be overridden by derived classes if they
        need to perform some field-specific initialization already
        before *production* is called. An example is adding a hook
        to the field.  The default implementation does nothing. 
        
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
        
    def productionForLiteral(self, field, literal):
        """XXXX""" 
        util.internal_error("Type.productionForLiteral() not overidden for %s" % self.__class__)
        
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

class Container(ParseableType):
    """A parseable type that is a container, i.e., a collection of items of a
    particular type. 
    
    Todo: Currently, there are no method in this class, we just use it as a
    trait to idenify containers. ~~List is the only container we have
    currently, but once we have more, we should move their shared methods
    (e.g., ~~itemType) to here.
    """
    pass
        
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

    ### Overidden from Type.
    
    def resolve(self, resolver):
        i = resolver.scope().lookupID(self._id)
        
        if not i:
            resolver.error(self, "undefined type id %s" % self._id)
            return self
        
        if not isinstance(i, id.Type):
            resolver.error(self, "identifier %s does not refer to a type" % self._id)
            return self 
        
        i.resolve(resolver)
        
        t = copy.copy(i.type())
        
        t._attrs = copy.copy(self._attrs)
        return t

    _type_name = "<unknown type>" 
    
class Identifier(Type):
    """Type for an unbound identifier.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Identifier, self).__init__(location=location)

    def pac(self, printer):
        printer.output("<type.Identifier>") # Should not get here.
        
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

    ### Overidden from Type.
    
    def doResolve(self, resolver):
        self._type = self._type.resolve(resolver)
        
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

