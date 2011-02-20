# $Id$

builtin_id = id

import copy

import node
import scope
import id
import type
import binpac.util as util
import binpac.pgen as pgen
import binpac.expr as expr
import binpac.stmt as stmt
import binpac.grammar as grammar
import binpac.operator as operator

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
        self._exported = False
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

    def _readDefaultAttribute(self, name):
        all = self.supportedAttributes()
        m = all[name]

        if len(m) == 3:
            (ty, const, default) = m
            explicit = False
        else:
            (ty, const, default, explicit) = m

        return (ty, const, default, explicit)

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
            (ty, const, default, explicit) = self._readDefaultAttribute(name)
        except KeyError:
            return None

        if default and not explicit:
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
            (ty, const, default, explicit) = self._readDefaultAttribute(name)
        except KeyError:
            return None

        if name in self._attrs:
            expr = self._attrs[name]
            if not expr and default and explicit:
                return default

            return expr

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

    def setExported(self):
        """Marks this type as exported, i.e., being defined by an ~~ID that
        has linkage ~~EXPORTED.
        """
        self._exported = True

    def exported(self):
        """Returns whether this type is exported, i.e., being defined by an
        ~~ID that has linkage ~~EXPORTED.

        Returns: bool - True if exported.
        """
        return self._exported

    def resolve(self, resolver):
        """Resolves any previously unknown types that might be
        referenced by this type. (Think: forward references). For
        these, the not-yet-known types will have been created as
        ~~Unknown; later, when the type should be known, this method
        will be called to then lookup the real type. If an error is
        encountered (e.g., we still don't the identifier), an error
        message will be reported via the *resolver*.

        This method *can* be overridden by derived classes if it wants replace
        an *instance* with something else. Note that if you only want
        recursively resolve further attributes in derived classes, you must
        not override this method but ~~_resolve instead.

        resolver: ~~Resolver - The current resolver to use.

        Return: ~~Type - Returns a type equivalent to *self* with
        all unknowns resolved; that may either be *self* itself, or
        a newly instantiated type; the caller must then use the
        returned type instead of *self* afterwards.

        Note: To implement resolving for derived classes, you must not
        override this method but instead ~~_resolve.

        Note: ~~Type is not derived from ~~Node and thus we are not
        overriding ~~Node.resolve here, even though it might
        initially look like that. This method does work similar but
        it returns a value, which ~~Node.resolve does not.
        """
        if self._resolved:
            return self

        self._resolved = True
        self._resolve(resolver)
        return self

    def __ne__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        return self.name()

    def canCoerceTo(self, dsttype):
        """Returns whether an expression of this type can be coerced to a
        given target type. If *dsttype* is of the same type as that of the
        current type, the result is always True.

        *dstype*: ~~Type - The target type.

        Returns: bool - True if the expression can be coerceed.
        """
        return operator.canCoerceTo(self, dsttype)

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

    def _resolve(self, resolver):
        """XXXX"""
        for (name, expr) in self._attrs.items():
            if expr:
                expr.resolve(resolver)

    def validate(self, vld):
        """Validates the semantic correctness of the type.

        Can be overridden by derived classes; the default implementation does
        some Type-specific stuff. If there are any errors encountered during
        validation, the method must call ~~Validator.error. If there are any
        sub-nodes that also need to be checked, the method needs to do that
        recursively.

        Derived classes should call their parent's implementation.

        vld: ~~Validator - The validator triggering the validation.
        """
        for (name, expr) in self._attrs.items():

            all = self.supportedAttributes()
            if name not in all:
                vld.error(self, "unknown type attribute &%s" % name)
                continue

            (ty, init, default, explicit) = self._readDefaultAttribute(name)

            if explicit and not expr and default:
                expr = default

            if isinstance(ty, str):
                i = vld.currentModule().scope().lookupID(ty)
                if i:
                    ty = i.type()
                else:
                    util.internal_error("unknown type %s defined for attribute %s" % (ty, name))

            if ty and not expr and (not default or not explicit):
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

        Returns: dictionary *name* -> (*type*, *init*, *default*) - *name* is
        a string with the attribute's name (excluding the leading ampersand);
        *type* is a ~~Type defining the type the attribute's expression must
        have (either as a ~~Type or string referecing a global ID which's type
        will be taken), or None if the attribute doesn't take a expression;
        *init* is a boolean indicating whether the attribute's expression must
        suitable for intializing a HILTI variable; *default* is a ~~Ctor
        expression with a default expression to be used if the attributes is
        not explicitly specified, or None if no such default; and *explicit*
        is False if a given default means that the attribute is treated as
        present if not given at all, and True if it must be given always
        explicitly even when just taking the default value. Note that
        *explicit* can be skipped, and will be assumed as *False* then.

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

        return self.name() == other.name()

class ParseableType(Type):
    """Type that can be parsed from an input stream. Only these types can be
    used inside a ~~Unit.

    params: list of any - The type's parameters.
    """
    def __init__(self, params=None, location=None):
        super(ParseableType, self).__init__(params=params, location=location)
        self._pgen = None
        self._field = None

    def field(self):
        """Return the unit field if such has been associated with the type.

        Returns: ~~Field - The field or None.
        """
        return self._field

    def setField(self, field):
        """Associates a unit ~~Field with the type.

        field: ~~Field - The field.
        """
        self._field = field

    def parserGen(self):
        """Returns the parser generator used for this type. Only valid once
        the grammar for this unit has been compiled.

        Returns: ~~ParserGen - The parser generator that compiled this unit.
        """
        return self._pgen

    def generateUnpack(self, cg, args, op1, op2, op3=None, callback=None):
        """Generates a HILTI ``unpack`` instruction wrapped in error handling
        code. The error handling code will do "the right thing" when either a
        parsing error occurs or insufficient input is found. In the latter
        case, it will execute the code generated by
        ~~generateInsufficientInputHandler.

        cg: ~~CodeGen - The current code generator.

        args: An ~~Args objects with the current parsing arguments, as
        used by the ~~ParserGen.

        op1, op2, op3: ~~hilti.Operand - Same as with the regular ``unpack``
        instruction.

        callback: function - A function that will be called for
        generating additional code for the case of insufficient input.

        Returns: ~~hilti.Operand - The result of the unpack.
        """

        fbuilder = cg.functionBuilder()

        parse = fbuilder.newBuilder("parse")
        cg.builder().jump(parse)

        try_ = fbuilder.newBuilder(None, add=False)
        error = fbuilder.newBuilder(None, add=False)
        insufficient = fbuilder.newBuilder(None, add=False)
        cont = fbuilder.newBuilder("cont")

        bytesit = hilti.type.IteratorBytes(hilti.type.Bytes())
        resultt = hilti.type.Tuple([self.hiltiType(cg), bytesit])
        result = fbuilder.addLocal("__unpacked", resultt)
        try_.unpack(result, op1, op2, op3)

        error.makeRaiseException("BinPAC::ParseError", "unpack failed")

        cg.setBuilder(insufficient)
        if callback:
            callback()
        iter = fbuilder.addTmp("__iter", bytesit)
        cg.builder().tuple_index(iter, op1, 0)
        cg.generateInsufficientInputHandler(args, iter=iter)
        cg.builder().jump(parse)

        catch1 = (None, [error.block()])
        catch2 = (fbuilder.idOp("Hilti::WouldBlock").type(), [insufficient.block()])

        ins = hilti.instructions.trycatch.TryCatch([try_.block()], [catch1, catch2])
        parse.block().addInstruction(ins)
        parse.jump(cont)

        cg.setBuilder(cont)
        return result

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
        return None

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

    def generateParser(self, cg, pgen, var, args, dst, skipping):
        """Generate code for parsing an instance of the type.

        The method must be overridden by derived classes which may be used as
        the type of ~~Variable production.

        cg: ~~CodeGen - The current code generator.

        var: ~~Variable - The variable being parsed.

        dst: hilti.operand.Operand - The operand in which to store
        the parsed value. The operand will have the type returned by
        ~~hiltiType.

        args: An ~~Args objects with the current parsing arguments, as
        used by the ~~ParserGen.

        skipping: boolean - True if the parsed value will actually never be
        used. If so, the function is free to skip fully parsing it, as well as
        storing anything in *dst*. It however must still return the advanced
        iterator.

        Returns: ~~hilti.operand.Operand - A a byte iterator
        containing the advanced parsing position, pointing to the first byte
        *not* consumed anymore.
        """
        util.internal_error("Type.production() not overidden for %s" % self.__class__)

class ParseableWithByteOrder(ParseableType):
    """Type that can be parsed from an input stream, with a binary
    representation that depends on byte order.

    See ~~Parseable for arguments.

    Note: This is primarily a helper class that provides some shared
    functionality.
    """
    def _isByteOrderConstant(self):
        expr = self._hiltiByteOrderExpr()
        return expr.isInit() if expr else True

    def _hiltiByteOrderExpr(self):
        order = self.attributeExpr("byteorder")
        if order:
            return order

        if self.field():
            prop = self.field().parent().property("byteorder", parent=self.field().parent().module())
            if prop:
                return prop

        return None

    def _hiltiByteOrderOp(self, cg, packeds, arg):
        e = self._hiltiByteOrderExpr()

        if e and not e.isInit():
            # TODO: We need to add code here that selects the right unpack type
            # at run-time.
            util.internal_error("non constant byte not yet supported")

        if e:
            assert isinstance(e, expr.Name)
            t = e.name().split("::")[-1].lower()

        else:
            # This is our default if no byte-order is specified.
            t = "big"

        try:
            enum = packeds[(t, arg)]
        except KeyError:
            cg.error("unknown type/byteorder combination")

        return cg.builder().idOp(enum)

class Iterable:
    """Trait class for a ~~Type that provides itertors."""

    ### Methods for derived classes to override.

    def iterType(self):
        """Returns the type of the type's iterators.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: ~~Iterator - The iterator type.
        """
        util.internal_error("Type.iterType() not overidden for %s" % self.__class__)

class Container(Type, Iterable):
    """A type that is a container, i.e., a collection of items of a particular
    type.

    ty: ~~Type or ~~Expr - The type of the container elements. If an
    expression, its type defines the item type.
    """
    def __init__(self, ty, location=None):
        super(Container, self).__init__(location=location)
        self._item = ty

    def itemType(self):
        """Returns the type of the container elements.

        Returns: ~~Type - The type of the container elements.
        """
        if isinstance(self._item, expr.Expression):
            self._item = self._item.type()

        return self._item.parsedType()

    def setItemType(self, ty):
        """XXX"""
        self._item = ty

    def _resolve(self, resolver):
        super(Container, self)._resolve(resolver)
        self._item = self._item.resolve(resolver)

    def validate(self, vld):
        super(Container, self).validate(vld)
        self._item.validate(vld)

class ParseableContainer(type.Container, type.ParseableType):
    """A parseable container type.

    ty: ~~Type or ~~Expr - The type of the container elements. If an
    expression, its type defines the item type.

    value: ~~Expr - If the container elements are specified via a constant,
    this expression gives that (e.g., regular expression constants for parsing
    bytes). None otherwise.

    itemargs: vector of ~~Expr - Optional parameters for parsing the items.
    Only valid if the item type is a ~~Unit.  None otherwise.

    location: ~~Location - A location object describing the point of
    definition.
    """
    def __init__(self, ty, value=None, item_args=None, location=None):
        super(ParseableContainer, self).__init__(ty, location=location)
        self._item_args = item_args
        self._prod = None
        self._value = value
        self._field = None

    ### Overridden from Type.

    def name(self):
        return "%s<%s>" % (self.typeName(), self.itemType().name())

    def _resolve(self, resolver):
        # Before we call the parent's method, let's see if our type
        # actually resolved to a constant.
        old_item = self.itemType()

        if isinstance(self.itemType(), type.Unknown):

            (val, ty) = resolver.lookupTypeID(self.itemType())

            self._value = val
            self.setItemType(ty)

        super(ParseableContainer, self)._resolve(resolver)

        self._item.copyAttributesFrom(old_item)

        if self._value:
            self._value.resolve(resolver)

        if self._item_args:
            for p in self._item_args:
                p.resolve(resolver)

    def validate(self, vld):
        super(ParseableContainer, self).validate(vld)

        if self._value:
            self._value.validate(vld)

        if self._item_args and not isinstance(self.itemType(), type.Unit):
            vld.error(self, "parameters only allowed for unit types")

        if self._item_args:
            for p in self._item_args:
                p.validate(vld)

    def validateCtor(self, vld, value):
        if not isinstance(value, list) and not isinstance(value, tuple):
            vld.error(self, "%s: ctor value of wrong internal type" % str(self))

        for elem in value:
            if not isinstance(elem, expr.Expression):
                vld.error(self, "%s: ctor value's elements of wrong internal type" % str(self))

            if elem.type() != self.itemType():
                vld.error(self, "%s: ctor value must be of type %s" % (str(self), elem.type()))

            elem.validate(vld)

    def pac(self, printer):
        printer.output("%s<" % self.__class__.typeName())
        self.itemType().pac(printer)
        printer.output(">")

    def pacCtor(self, printer, elems):
        printer.output("%s<" % self.__class__.typeName())
        self.itemType().pac(printer)
        printer.output(">(")
        for i in range(len(elems)):
            self.itemType().pacCtor(printer, elems[i])
            if i != len(elems) - 1:
                printer.output(", ")
        printer.output(")")

    ### Overridden from ParseableType.

    def initParser(self, field):
        self._field = field

    def _itemProduction(self, field):
        """Todo: This is pretty ugly and mostly copied from
        ~~unit.Field.production. We should probably use a ~~unit.Field as our
        item type directly."""
        if self._value:
            for t in type._AllowedConstantTypes:
                if isinstance(self.itemType(), t):
                    prod = grammar.Literal(None, self._value, location=self._location)
                    break
            else:
                util.internal_error("unexpected constant type for literal")

        else:
            prod = self.itemType().production(field)
            assert prod

            if self._item_args:
                prod.setParams(self._item_args)

        return prod

    def containerProduction(self, field, item):
        """XXXX"""
        util.internal_error("not overridden")

    def production(self, field):
        if self._prod:
            return self._prod

        loc = self.location()
        item = self._itemProduction(field)
        item.setForEachField(field)
        unit = field.parent()

        if field.name():
            # Create a high-priority hook that pushes each element into the
            # container as it is parsed.
            hook = stmt.FieldForEachHook(unit, field, 254)

            # Create a "vector.push_back($$)" statement for the internal_hook.
            loc = None
            dollar = expr.Name("__dollardollar", hook.scope(), location=loc)
            slf = expr.Name("__self", hook.scope(), location=loc)
            vector = expr.Attribute(field.name(), location=loc)
            method = expr.Attribute("push_back", location=loc)
            attr = expr.Overloaded(operator.Operator.Attribute, (slf, vector), location=loc)
            push_back = expr.Overloaded(operator.Operator.MethodCall, (attr, method, [dollar]), location=loc)
            push_back = stmt.Expression(push_back, location=loc)

            hook.setStmts(stmt.Block(None, stmts=[push_back]))
            unit.module().addHook(hook)

        self._prod = self.containerProduction(field, item)
        return self._prod

class Iterator(Type):
    """Type for iterating over elements stored by an instance of another type.

    t: ~~Type - The type storing the element we are iterating over. That type
    must be an ~~Iterable.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, t, location=None):
        super(Iterator, self).__init__(location)
        self._type = t

    def parentType(self):
        """Returns the type storing the elements being iterated over.

        Returns: ~~Type - The type.
        """
        return self._type

    # Methods for derived classes to override.

    def derefType(self):
        """Returns the type of elements being iterated over.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: ~~Type - The type.
        """
        util.internal_error("Iterator.derefType() not overidden for %s" % self.__class__)

    ### Overridden from Type.

    def name(self):
        return "iterator<%s>" % self._type

    def _resolve(self, resolver):
        self._type = self._type.resolve(resolver)

    def validate(self, vld):
        self._type.validate(vld)

        if not isinstance(self._type, Iterable):
            vld.error(self._type, "iterator requires an iterable tyype as parameter")

class Sinkable:
    """Trait class for a ~~Type to which a ~~Sink can directly be attached.

    If a type is Sinkable, it needs to call one of the method
    ~~hiltiWriteDataToSink or ~~hiltiWriteRangeToSink when it has a sink
    attached (~~UnitField.sink) and data available.
    """

    def hiltiWriteDataToSink(self, cg, sink, data):
        """Writes a bytes object into a sink.

        cg: ~~CodeGen - The code generator to use.

        sink: ~~Expr - An expression evaluating to the target sink.

        data: ~~hilti.Operand - An operand with the data to write, which much
        be of type ~~Bytes.
        """
        assert isinstance(sink.type(), type.Sink)
        assert isinstance(data.type(), hilti.type.Reference) and isinstance(data.type().refType(), hilti.type.Bytes)

        sink = sink.evaluate(cg)
        user = cg.builder().idOp("__user")

        cfunc = cg.builder().idOp("BinPAC::sink_write")
        cargs = cg.builder().tupleOp([sink, data, user])
        cg.builder().call(None, cfunc, cargs)

    def hiltiWriteRangeToSink(self, cg, sink, begin, end):
        """Writes a bytes range into a sink.

        cg: ~~CodeGen - The code generator to use.

        sink: ~~Expr - An expression evaluating to the target sink.

        begin: ~~hilti.Operand - An operand with a bytes iterator defining
        the beginning of the data to write into the sink.

        end: ~~hilti.Operand - An operand with a bytes iterator defining
        the end of the data to write into the sink.
        """
        sink = sink.evaluate(cg)

        data = cg.builder().addLocal("__data", hilti.type.Bytes())
        cg.builder().bytes_sub(data, begin, end)

        self.hiltiWriteDataToSink(cg, sink, data)

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
        # Yes, we want to override resolve() here (not _resolve).
        i = resolver.scope().lookupID(self._id)

        if not i:
            resolver.error(self, "undefined type id %s" % self._id)
            return self

        if not isinstance(i, id.Type) and not isinstance(i.type(), type.RegExp):
            resolver.error(self, "identifier %s does not refer to a type" % self._id)
            return self

        i.resolve(resolver)

        t = i.type() # i.copy(i.type()) XXX

        t._attrs = copy.copy(self._attrs)

        # Still calling _resolve though.
        self._resolve(resolver)

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

    def _resolve(self, resolver):
        self._type = self._type.resolve(resolver)

    def validate(self, vld):
        self._type.validate(vld)

    def pac(self, printer):
        self._type.output(printer)

    def hiltiType(self, cg):
        util.internal_error("hiltiType() not defined for MetaType")

    _type_name = "<meta type>"

class Any(Type):
    """Type indicating a match with any other type.
    """
    pass

class Void(Type):
    """Type indicating a void function/method return value.
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

