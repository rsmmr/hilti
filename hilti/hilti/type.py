# $Id$

builtin_type = type
builtin_id = id

import llvm.core

import node
import types
import inspect
import id
import util

def _notOverridden(self, method):
    util.internal_error("%s not overidden for %s" % (method, self.__class__.__name__))

def hilti(token, ident, c=None, hdr=None, doc=None):
    """Class decorator to add a type class that is defined in some other
    module to the namespace of *hilti.type*.

    token: string or list of string - A token for the type which the
    parser will recognize; or None if none is needed. If given, it
    that token will also be the default return value of ~~typeName.

    ident: inter - An integer unique across all types; the integer
    must have a matching constant ``__HLT_TYPE_*`` defined in
    :download:`/libhilti/hilti_intern.h`. Can be -1 if and only if
    the type will never be used by any C function.

    c: string - The C prototype to use for this type, or None if none. This
    prototype applies all instances of the type class; a per-instance
    prototype can be defined in the type's ~~TypeInfo, which will then
    override this. 

    hdr: - The name of the C header file associated with this tyoe, or None if
    none. This information will be used for the generated documentation.

    doc: string - The string to be used for this type in the auto-generated
    documentation. This should include the right cross-reference, such as in
    ```:hlt:type:\`net\``

    Todo: Perhaps we should autogenerate the *_id* constants in some
    way and also automatically synchronize them with
    ``hilti_intern.h``.
    """
    def _hilti(ty):
        import hilti
        hilti._types[ty.__name__] = ty
        globals()[ty.__name__] = ty

        ty._token = token
        ty._id = ident
        ty._doc_name = doc
        ty._c_prototype = c
        ty._c_hdr = hdr

        if token:
            ty._type_name = token
        else:
            ty._type_name = ty.__name__

        return ty

    return _hilti

def allTypes():
    """Returns a list of all defined types.

    Returns: list of ~~Type - The types. 
    """
    import hilti
    return hilti._types.values()

### Base classes

class Type(object):
    """Base class for all data types provided by the HILTI language.

    location: ~~Location - The location where the type was defined.
    """

    _doc_name = None

    def __init__(self, location=None):
        super(Type, self).__init__()
        Type._type_name = "type"
        self._location = location
        self._internal_name = None

    def location(self):
        """Returns  location information for the type.

        Returns: ~~Location - The location where the type was defined."""
        return self._location

    def setInternalName(self, name):
        """Sets the internal C name for this type. If this is set, it is
        assumed that the type is defined externally and references to it will
        be replaced with this name.

        name: string - The internal name.
        """
        self._internal_name = name

    def internalName(self):
        """Returns the internal name of this type.  If this is set, it is
        assumed that the type is defined externally and references to it will
        be replaced with this name.

        Returns: string or None - The name, or None if none has been set.
        """
        return self._internal_name

    def resolve(self, resolver):
        """Resolves any previously unknown types that might be referenced by
        this type. (Think: forward references). For these, the not-yet-known
        types will have been created as ~~Unknown; later, when the type should
        be known, this method will be called to then lookup the real type. If
        an error is encountered (e.g., we still don't the identifier), an
        error message will be reported via the *resolver*.

        resolver: ~~Resolver - The current resolver to use.

        Returns: ~~Type - Returns a type equivalent to *self* with all unknowns
        resolved; that may either be *self* itself, or a newly instantiated
        type; the caller must then use the returned type instead of *self*
        afterwards.

        Note: ~~Type is not derived from ~~Node and thus we are not overriding
        ~~Node.resolve here, even though it might initially look like that.
        This method does work similar but it returns a value, which
        ~~Node.resolve does not.

        Note: To implement resolving for derived classes, do not
        override this method but ~~_resolve.
        """
        return self._resolve(resolver)

    def validate(self, vld):
        """Validates the semantic correctness of the type.

        vld: ~~Validator - The validator triggering the validation.

        Note: ~~Type is not derived from ~~Node and thus we are not overriding
        ~~Node.validate here, even though it might initially look like that.

        Note: To implement validation for derived classes, do not
        override this method but ~~_validate.
        """
        return self._validate(vld)

    def autodoc(self):
        pass

    @classmethod
    def token(cls):
        """Returns the parser token for the type. This is what's passed to
        ~~type.hilti.

        Returns: string - The token string.
        """
        return cls._token

    @classmethod
    def typeName(cls):
        """Returns a short, descriptive name for instances of the type. These
        names can be used in messages to the user. They are class-wide, i.e.,
        the same for all instances of a type class.

        Per default, the returned name is what's defined by the ~~hilti
        decorator. The value can be changed with ~~setTypeName.
        """
        return cls._type_name

    @classmethod
    def setTypeName(cls, name):
        """Sets the type name for a type class. See ~~typeName.

        name: string - The name.
        """
        cls._type_name = name

    @classmethod
    def cPrototype(cls):
        """Returns the C prototype for this type.

        Returns: string - The C type the HILTI type is mapped to. None if not set.
        """
        #assert cls._c_prototype
        return cls._c_prototype

    @classmethod
    def cHeader(cls):
        """Returns the name fo the C header associated with this type.

        Returns: string - The name of the C header.
        """
        #assert cls._c_prototype
        return cls._c_hdr

    def __str__(self):
        """Returns a string representation of the type. This returns
        ~~name, which can be overridden to change.

        Returns: string - The type's name.
        """
        return self.name()

    ### Methods for derived classes to override.

    def name(self):
        """Returns the name of the type. This name may include type-specific
        parameters.

        Can be overridden by derived classes; the default implementation
        returns ~~typeName.

        Returns: string - The name of the type.
        """
        return self.__class__.typeName()

    @classmethod
    def docName(cls):
        """Returns the name of the type as used in the instruction
        documentation.

        Can be overridden by derived classes; the default implementation
        returns ~~typeName.

        Returns: string - The documentation name of the type.
        """
        return cls._doc_name if cls._doc_name else ":hlt:type:`%s`" % cls.typeName()

    @classmethod
    def setDocName(cls, name):
        """Sets the name of the type as used in the instruction documentation.

        name: string - The name.
        """
        cls._doc_name = name

    def _resolve(self, resolver):
        """Implements resolving for derived classes.

        Can be overridden by derived classes; the default implementation just
        returns self. Derived classes should *not* call their parent's method.

        resolver: ~~Resolver - The current resolver to use.

        Returns: ~~Type - Returns a type equivalent to *self* with all unknowns
        resolved; that may either be *self* itself, or a newly instantiated
        type; the caller must then use the returned type instead of *self*
        afterwards.
        """
        return self

    def _validate(self, vld):
        """Implements validation for derived classes.

        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any subnodes that
        also need to be checked, the method needs to do that recursively.

        Derived classes should call their parent's method.

        vld: ~~Validator - The validator triggering the validation.
        """
        return

    def output(self, printer):
        """Converts the type into parseable HILTI code.

        Can be overidden by derived classes; the default implemenation
        outputs ~~name.

        Derived classes should *not* call their parent's method.

        printer: ~~Printer - The printer to use.
        """

        # FIXME: Below should really be defined in Paramterizable. However, the
        # function doesn't get called then, presumably because Paramteriazalb is
        # not derived from Type?
        if isinstance(self, Parameterizable):
            printer.output("%s<" % self._token)

            first = True
            for i in self.args():
                if not first:
                    printer.output(",")

                if isinstance(i, Type):
                    printer.printType(i)
                else:
                    if isinstance(i, node.Node):
                        i.output(printer)
                    else:
                        printer.output(str(i))

                first = False

            printer.output(">")

        else:
            printer.output(self.name())

    def cmpWithSameType(self, other):
        """Compares with another objects of the same class. The default
        implementation compares the results of the two instances' ~~name
        methods. Derived classes can override the method to implement their
        own type checking.

        Returns: bool - True if the two objects are equivalent.
        """
        assert self.__class__ == other.__class__
        return self.name() == other.name()

    def __eq__(self, other):
        """Compares two types for equivalance.

        Todo: Document how exactly we do this.

        Todo: We should overhaul the type-checking though. Do we still need
        this now that we do the coercion logic?

        Todo: Partially. The equality should True iff two types do not need
        any coercion either way for passing around, i.e., they are fully
        equivalent.
        """

        # Special case for tuples/list: one of the members has to match.
        if builtin_type(other) == types.ListType or builtin_type(other) == types.TupleType:
            for t in other:
                if self == t:
                    return True

            return False

        # Special case for classes: if we're an instance of that class, we're
        # fine.
        if inspect.isclass(other):
            return _matchWithTypeClass(self, other)

        # We match any Any instance.
        if isinstance(other, Any):
            return True

        # If we are Any, we match everything else.
        if isinstance(self, Any) and isinstance(other, Type):
            return True

        # If we're the same kind of thing, ask the derived class whether we
        # match; except for the wildcard types which we handle directly.
        if builtin_type(self) == builtin_type(other):
            return self.cmpWithSameType(other)

        # Comparision with any other type has failed now.
        if isinstance(other, Type):
            return False

        # Can't tell.
        return NotImplemented

    def __ne__(self, other):
        eq = self.__eq__(other)
        return not eq if eq != NotImplemented else NotImplemented

    _id = 0 # Zero is used as an error indicator.


class HiltiType(Type):
    """Base class for all HILTI types that can be directly instantiated in a
    HILTI program. That includes use in global and local variables, as well as
    allocation on the heap.

    The compiler generates type information for all HILTI types.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(HiltiType, self).__init__(location)
        HiltiType.setTypeName("<HILTI Type>")

    ### Methods for derived classes to override.

    def typeInfo(self, cg):
        """Returns type information for the type.

        Must be overridden by derived classes.

        The doc string of derived classes should document any specifics for
        the type's type information, such as usage of the *aux* field.

        cg: ~~CodeGen - The current code generator.

        Returns: ~~TypeInfo - An object with fields suitable filled.
        """
        _notOverridden(self, "typeInfo")

    def llvmType(self, cg):
        """Returns the corresponding LLVM type.

        Must be overridden by derived classes.

        The doc string of derived classes can document how the type is mapped
        to C; if not given, the information is derived from the ~~TypeInfo
        object.

        cg: ~~CodeGen - The current code generator.

        Returns: llvm.core.Type - The LLVM type.
        """
        _notOverridden(self, "llvmType")

    def cPassTypeInfo(self):
        """Returns whether type information needs to be given to C functions
        when an instance of this type is passed into it. If so, the C argument
        corresponding to the instance will be preceded by an argument of type
        ``hlt_type_info*``.

        May be overridden by derived classes. The default implementation
        always returns False.

        Returns: bool - True type information must be provided.
        """
        return False

    def canCoerceTo(self, dsttype):
        """Returns whether a value of this type can be implicitly coerced to a
        given target type.

        Can be overridden by derived classes; if so, the method must always
        returns True for equivalent types that do not need any coercion, as
        determined by the equality operator. The default implementation
        returns True only in that case.  Derived classes can call their
        parent's implementation if they need that's coercion behavior, but
        they don't need to.

        *dstype*: ~~Type - The target type.

        Returns: bool - True if the types can be coerced.

        Note: Do not call this method directly but use ~~type.canCoerceTo
        instead.
        """
        return self == dsttype

    def canCoerceFrom(self, srctype):
        """Returns whether we can implicitly coerce a value of another type
        into this type.

        Can be overridden by derived classes; if so, the method must always
        returns True for equivalent types that do not need any coercion, as
        determined by the equality operator. The default implementation
        returns True only in that case.  Derived classes can call their
        parent's implementation if they need that's coercion behavior, but
        they don't need to.

        *srctype*: ~~Type - The target type.

        Returns: bool - True if the types can be coerced.

        Note: Do not call this method directly but use ~~type.canCoerceTo
        instead.
        """
        return self == srctype

    def llvmCoerceTo(self, cg, value, dsttype):
        """Coerces a value to a different type. The value must be an LLVM
        value generated by an ~~Operand of type *self*. The method must only
        be called if ~~canCoerceTo indicates that coercion is possible.

        Can be overridden by derived classes; if so, the method must return
        ``value`` for equivalent types that do not need any coercion, as
        determined by the equality operator.  The default implementation
        handles only this case. Derived classes can call their parent's
        implementation if they need that's coercion behavior, but they don't
        need to.

        *cg*: ~~CodeGen - The current code generator.

        *value*: ~~llvm.core.Value - The value to cast.

        *dsttype*: ~~Type - The type to cast the expression into.

        Returns: ~~llvm.core.Value - The casted value.

        Note: Do not call this method directly but use ~~type.llvmCoerceTo
        instead.
        """
        assert self.canCoerceTo(dsttype)
        return value

    def llvmCoerceFrom(self, cg, value, srctype):
        """Coerces a value of a different type to this one. The value must be
        an LLVM value generated by an ~~Operand of type *self*. The method
        must only be called if ~~canCoerceFrom indicates that coercion is
        possible.

        Can be overridden by derived classes; if so, the method must return
        ``value`` for equivalent types that do not need any coercion, as
        determined by the equality operator.  The default implementation
        handles only this case. Derived classes can call their parent's
        implementation if they need that's coercion behavior, but they don't
        need to.

        *cg*: ~~CodeGen - The current code generator.

        *value*: ~~llvm.core.Value - The value to cast.

        *dsrctype*: ~~Type - The type to cast the expression into.

        Returns: ~~llvm.core.Value - The casted value.

        Note: Do not call this method directly but use ~~type.llvmCoerceTo
        instead.
        """
        assert self.canCoerceFrom(srctype)
        return value

# HiltiType traits.

class Trait:
    @classmethod
    def docName(cls):
        """Returns the name of the type as used in the instruction
        documentation.

        Can be overridden by derived classes; the default implementation
        returns the lower-cased class name. 

        Returns: string - The documentation name of the type.
        """
        return cls.__name__.lower()

class Constructable(Trait):
    """Trait class for ~~HiltiTypes that allow the creation of *non-const*
    instances with a constructor expression."""

    def validateCtor(self, vld, value):
        """Validates the semantic correctness of the ctor expressions.

        Must be overidden by derived classes. Derived class should call their
        parent's implementation first.

        The doc string of derived classes should document the HILTI syntax of
        constructor expressions for this type.

        vld: ~~Validator - The validator triggering the validation.

        value: any - The value of the ctor expression, with a type-specific type.
        """
        _notOverridden(self, "validateCtor")

    def llvmCtor(self, cg, value):
        """Returns the LLVM value for a ctor expression.

        Must be overidden by derived classes.  Must be overidden by derived
        classes. Derived class should *not* call their parent's
        implementation.

        cg: ~~CodeGen - The current code generator.

        value: any - The value of the ctor expression, with a type-specific type.

        Returns: llvm.core.Value - The created LLVM value. The type must match
        with what ~~llvmType returns.
        """
        _notOverridden(self, "llvmCtor")

    def outputCtor(self, printer, const):
        """Converts a ctor of the type into its HILTI source code
        representation.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        printer: ~~Printer - Th printer to use.
        value: any - The value of the ctor expression, with a type-specific type.
        """
        _notOverridden(self, "outputCtor")

    def canCoerceCtorTo(self, ctor, dsttype):
        """Returns whether a constructor of this type can be implicitly
        coerced to a given target type.

        Can be overridden by derived classes; if so, the method must
        always returns True for equivalent types that do not need
        any coercion, as determined by the equality operator. The
        default implementation returns True only in that case.
        Derived classes can call their parent's implementation if
        they need that's coercion behavior, but they don't need to.

        *ctor*: ~~Constant - The constructor to cast.

        *dstype*: ~~Type - The target type.

        Returns: bool - True if the constructor can be casted.

        Note: Do not call this method directly but use
        ~~type.canCoerceConstantTo instead.
        """
        return self == dsttype

    def coerceCtorTo(self, cg, ctor, dsttype):
        """Coerces a constructor to a different type. The value must be
        an LLVM value generated by an ~~Operand of type *self*. The
        method must only be called if ~~canCoerceTo indicates that
        coercion is possible.

        Can be overridden by derived classes; if so, the method must
        return ``value`` for equivalent types that do not need any
        coercion, as determined by the equality operator.  The
        default implementation handles only this case. Derived
        classes can call their parent's implementation if they need
        that's coercion behavior, but they don't need to.

        *ctor*: ~~Constant - The constructor to cast.

        *srctype*: ~~Type - The type to cast the expression from.

        Returns: ~~Constant - The casted constructor.

        Note: Do not call this method directly but use ~~type.coerceConstantTo
        instead.
        """
        assert self.canCoerceCtorTo(ctor, dsttype)
        return ctor if self == dsttype else None

class Constable(Trait):
    """Trait class for ~~HiltiTypes that allow the instantion of constants."""

    def resolveConstant(self, resolver, const):
        """Resolves any previously unknown types that might be referenced by
        this constant. This methods works similarly to ~~ast.resolve; see
        there for more information.

        Can be overidden by derived classes. The default implementation does
        nothing. Derived class should call their parent's implementation
        first.

        resolver: ~~Resolver - The resolver to use.
        const: ~~Constant - The constant to resolve.

        Note: This *really* behaves like ~~ast.resolve, not Type.~~resove. It
        does not return a new constant but changes the passed in constant in
        place.
        """
        pass

    def validateConstant(self, vld, const):
        """Validates the semantic correctness of a constant of the type.

        Must be overidden by derived classes. Derived class should call their
        parent's implementation first.

        The doc string of derived classes should document the HILTI syntax for
        constants of this type.

        vld: ~~Validator - The validator triggering the validation.
        const: ~~Constant - The constant, which will have the type's type.
        """
        _notOverridden(self, "validateConst")

    def llvmConstant(self, cg, const):
        """Returns the LLVM constant for a constant of this type.

        Must be overidden by derived classes.  Derived class should *not* call
        their parent's implementation first.

        cg: ~~CodeGen - The current code generator.
        const: ~~Constant - The constant to convert to HILTI.

        Returns: llvm.core.Constant - The LLVM constant. The type must match
        with what ~~llvmType returns.
        """
        _notOverridden(self, "llvmConstant")

    def canCoerceConstantTo(self, const, dsttype):
        """Returns whether a constant of this type can be implicitly
        coerced to a given target type.

        Can be overridden by derived classes; if so, the method must
        always returns True for equivalent types that do not need
        any coercion, as determined by the equality operator. The
        default implementation returns True only in that case.
        Derived classes can call their parent's implementation if
        they need that's coercion behavior, but they don't need to.

        *const*: ~~Constant - The constant to cast.

        *dstype*: ~~Type - The target type.

        Returns: bool - True if the constant can be casted.

        Note: Do not call this method directly but use
        ~~type.canCoerceConstantTo instead.
        """
        return self == dsttype

    def coerceConstantTo(self, cg, const, dsttype):
        """Coerces a constant to a different type. The value must be
        an LLVM value generated by an ~~Operand of type *self*. The
        method must only be called if ~~canCoerceTo indicates that
        coercion is possible.

        Can be overridden by derived classes; if so, the method must
        return ``value`` for equivalent types that do not need any
        coercion, as determined by the equality operator.  The
        default implementation handles only this case. Derived
        classes can call their parent's implementation if they need
        that's coercion behavior, but they don't need to.

        *const*: ~~Constant - The constant to cast.

        *srctype*: ~~Type - The type to cast the expression from.

        Returns: ~~Constant - The casted constant.

        Note: Do not call this method directly but use ~~type.coerceConstantTo
        instead.
        """
        assert self.canCoerceTo(dsttype)
        return const if self == dsttype else None

    def canCoerceConstantFrom(self, const, srctype):
        """Returns whether we can implicitly coerce a const of another type
        into this type.

        Can be overridden by derived classes; if so, the method must
        always returns True for equivalent types that do not need
        any coercion, as determined by the equality operator. The
        default implementation returns True only in that case.
        Derived classes can call their parent's implementation if
        they need that's coercion behavior, but they don't need to.

        *const*: ~~Constant - The constant to cast.

        *dstype*: ~~Type - The target type.

        Returns: bool - True if the constant can be casted.

        Note: Do not call this method directly but use
        ~~type.canCoerceConstantTo instead.
        """
        return self == srctype

    def coerceConstantFrom(self, cg, const, srctype):
        """Coerces a constant of a different type to this one. The value must
        be an LLVM value generated by an ~~Operand of type *self*. The method
        must only be called if ~~canCoerceFrom indicates that coercion is
        possible.

        Can be overridden by derived classes; if so, the method must
        return ``value`` for equivalent types that do not need any
        coercion, as determined by the equality operator.  The
        default implementation handles only this case. Derived
        classes can call their parent's implementation if they need
        that's coercion behavior, but they don't need to.

        *const*: ~~Constant - The constant to cast.

        *srctype*: ~~Type - The type to cast the expression from.

        Returns: ~~Constant - The casted constant.

        Note: Do not call this method directly but use ~~type.coerceConstantTo
        instead.
        """
        assert self.canCoerceTo(srctype)
        return const if self == srctype else None

    def outputConstant(self, printer, const):
        """Converts a constant of the type into its HILTI source code
        representation.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        printer: ~~Printer - Th printer to use.
        const: ~~Constant - The constant to print.
        """
        _notOverridden(self, "outputConstant")

class Parameterizable(Trait):
    """Trait class for ~~HiltiTypes which take type parameters."""

    ### Methods for derived classes to override.

    def args(self):
        """Returns the type's parameters.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: list of ~~Operand - The paramters.
        """
        _notOverridden(self, "args")

class Iterable(Trait):
    """Trait class for ~~HiltiTypes which provide itertors."""

    ### Methods for derived classes to override.

    def iterType(self):
        """Returns the type of the type's iterators.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: ~~Iterator - The iterator type.
        """
        _notOverridden(self, "iterType")

class Unpackable(Trait):
    """A type that can be unpacked via the ``unpack`` operator."""

    ### Methods for derived classes to override.

    def formats(self, mod):
        """Returns the supported unpack formats.

        mod: ~~Module - The current module. Can be used to lookup
        other types in its scope.

        Must be overridden by derived classes.  Derived class should *not*
        call their parent's implementation.

        Returns: list of tuples (string, ~~Type, bool, string) - The supported
        formats. The first element of each tuple is the fully-qualified name
        of a ``Hilti::Packed`` constant (i.e., ``Hilti::Packed::Bool``); the
        second element the type required for the auxiliary ``unpack``
        argument, or None if none; the third specifies whether the
        auxiliary argument is optional; and the fourth gives a description of
        the format suitable for including in the HILTI reference documentation.

        The doc string of derived classes can have a doc string documenting
        further specifics of the unpacking. They do not need to document the
        supported enum types, nor their arguments; these are autogenerated
        from what the function returns.

        Note: The unpack operator already checks whether its format arguments
        match what this method returns; there's no need for a custom
        *validate* method doing that.
        """
        _notOverridden(self, "formats")

    def llvmUnpack(self, cg, begin, end, fmt, arg):
        """Implemens the ~~Unpack operator for a specific type.

        Must be overridden by derived classes. The implementation must emit
        LLVM code instantiating an object of type *t* from binary data stored
        in a ~~Bytes object. The method does not need to consume all bytes
        within the given range but it must not consume any beyond *end*.

        Also note that all unpacking functions must be able to deal with bytes
        located at arbitrary, and not necessarily aligned, positions.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        cg: ~~CodeGen - The code generator to use.

        begin: llvm.core.value - An iterator defining the starting position
        for unpacking.

        end: llvm.core.value - An iterator defining the end position for
        unpacking.

        fmt: ~~Operand - An operand of type ``Hilti::Packed`` determining the
        binary format. If ~~formats returns multiple possible options, you
        will probably build a switch statement with these via ~~llvmSwitch.

        arg: ~~Operand - The unpacks additional, optional argument, or None if
        none needed. Behaviour is undefined if *op* is not the type returned
        by ~~formats.

        Returns: Tuple (llvm.core.value, llvm.core.value) - The first element
        is the unpacked value; the second is a byte iterator pointing one
        beyond the last consumed input byte (which can't be further than
        *end*).

        Note: Different from most other operators, unpacking comes with its
        own ~~Type trait class. This is because it is also used internally,
        not only via the corresponding HILTI instruction. The trait class
        allows to use a single implementation for all use-cases.
        """
        _notOverridden(self, "unpack")

class Classifiable(Trait):
    """Trait class for ~~HiltiTypes which can used in ~~Classifier rules."""

    ### Methods for derived classes to override.

    def llvmToField(self, cg, ty, llvm_val):
        """Returns a LLVM value representing a rule field of value *llvm_val*.
        *llvm_val* can have any of the (corrsponding LLVM) types indicated as
        valid by ~~matchableTo. The returned value must be of C type
        ``hlt_classifier_field``.

        Must be overriden by derived classes.

        ty: ~~HiltiType - The original type of the LLVM value.

	    llvm_val: llvm.core.Value - An LLVM value to convert into the internal
        format.

        Returns:: llvm.core.Value - The field value.
        """
        _notOverridden(self, "llvmToField")

class TypeListable(Trait):
    """Trait class for ~~HiltiTypes which define an ordered list of types."""

    def typeList(self):
        """Returns the ordered list of types defined by this type.

        Must be overriden by derived classes.

        Returns: list of ~~HiltiType - The list.
        """
        _notOverridden(self, "typeList")

# End of traits for HiltiType.

class ValueType(HiltiType):
    """Base class for all types that can be directly stored in a HILTI local
    or global variable. Types derived from ValueType cannot be allocated on
    the heap.

    ValueTypes can be used as indices in maps and sets.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(ValueType, self).__init__(location)
        ValueType.setTypeName("<value type>")

    def llvmDefault(self, cg):
        """Returns the default value to initialize HILTI variables of this
        type with. The default must be a constant.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        The doc string of derived classes should document the default value of
        not explicitly initiallized HILTI variables of this type.

        *cg*: ~~CodeGen - The current code generator.

        Returns: llvm.core.Constant - The LLVM constant. The type must match
        with what ~~llvmType returns.
        """
        _notOverridden(self, "llvmDefault")

    ### Methods for derived classes to override.

    def instantiable(self):
        """Returns true if instances of this type can be instantiated by a
        HILTI program itself. The default implementation always returns True,
        which should be correct for most cases. However, derived classes can
        override this to be more selective (e.g, we can't create variable of
        type ``any`` or ``ref<*>``.

        May be overridden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: boolean - See above.
        """
        return True

class HeapType(HiltiType):
    """Base class for all types that must be allocated on the heap. Types
    derived from HeapType cannot be stored directly in variables and only
    accessed via a ~~Reference.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(HeapType, self).__init__(location)
        HeapType.setTypeName("<heap type>")

class Container(HeapType, Iterable, Parameterizable):
    """Base class for all container types. A container is iterable and stores
    elements of a certain *item type*.

    t: ~~Type or string ``*`` - The type of elements stored inside the
    container. ``*`` is for a wildcard type, ~~Type otherwise.

    Todo: Do we really need the ``*`` or can we just use None for that?
    """
    def __init__(self, t, location=None):
        if t and t != "*":
            util.check_class(t, Type, "Container")

        super(HeapType, self).__init__(location)
        self._type = t if t != "*" else None
        Container.setTypeName("<container>")

    def itemType(self):
        """Returns the type of the container values.

        Returns: ~~ValueType - The type of the container items, or None for
        the wildcard (``*``).
        """
        return self._type

    ### Overridden from Type.

    def name(self):
        return "%s<%s>" % (self._token, self._type if self._type else "*")

    def _resolve(self, resolver):
        if self._type:
            self._type = self._type.resolve(resolver)

        return self

    def _validate(self, vld):
        if self._type:
            self._type.validate(vld)

    ### Overriden from HiltiType.

    def cPassTypeInfo(self):
        return not self._type

    def canCoerceTo(self, dsttype):
        if isinstance(dsttype, Any):
            return True

        if not isinstance(dsttype, self.__class__):
            return False

        if not dsttype._type:
            return True

        return self._type == dsttype._type

    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)

        if isinstance(dsttype, Any):
            return value

        if not dsttype._type:
            return cg.builder().bitcast(value, cg.llvmTypeGenericPointer())

        return value

    ### Overriden from Parameterizable.

    def args(self):
        return [self._type]


class Iterator(ValueType, Parameterizable):
    """Type for iterating over elements stored by the instance of another type.

    t: ~~HiltiType - The type storing the element we are iterating over.

    location: ~~Location - The location where the type was defined.
    """

    def __init__(self, t, location=None):
        super(Iterator, self).__init__(location)
        Iterator.setTypeName("iterator<%s>" % t.typeName())
        self._type = t

    def derefType(self):
        """Returns the type of elements being iterated over.

        Must be overidden by derived classes. Derived class should *not* call
        their parent's implementation.

        Returns: ~~ValueType - The type.
        """
        _notOverridden(self, "derefType")

    def parentType(self):
        """Returns the type storing the elements being iterated over.

        Returns: ~~HiltiType - The type.
        """
        return self._type

    ### Overridden from Type.

    def name(self):
        return "iterator<%s>" % self._type

    def _resolve(self, resolver):
        self._type = self._type.resolve(resolver)
        return self

    def _validate(self, vld):
        self._type.validate(vld)

        if not isinstance(self._type, HiltiType):
            vld.error(self._type, "iterator requires hilti type as parameter")

    def output(self, printer):
        printer.output("iterator<")
        printer.printType(self._type)
        printer.output(">")

    ### Overridden from HiltiType.

    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, Iterator):
            return False

        return canCoerceTo(self.derefType(), dsttype.derefType())

    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)
        return llvmCoerceTo(cg, value, self.derefType(), dsttype.derefType())

    ### Overridden from Parameterizable.

    def args(self):
        return [self._type]

#### Actual types not defined elsewhere.

@hilti("void", -1)
class Void(ValueType, Constable):
    """Type representing a non-existing function result.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(Void, self).__init__(location)
        Void.setTypeName("void")

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """Void is, naturally, mapped to a C void."""
        return llvm.core.Type.void()

    ### Overridden from ValueType.

    def instantiable(self):
        # Never.
        return False

@hilti("any", 24, c="void *")
class Any(ValueType, Constable, Constructable):
    """Wildcard type that matches any other type.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(Any, self).__init__(location)
        Any.setTypeName("any")

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        return typeinfo

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """Parameters are always passed with type information."""
        return llvm.core.Type.pointer(llvm.core.Type.int(8))

    def cPassTypeInfo(self):
        # Always pass type infromation
        return True

    def canCoerceFrom(self, srctype):
        return isinstance(srctype, HiltiType)

    def llvmCoerceFrom(self, cg, value, srctype):
        return value

    ### Overridden from ValueType.

    def instantiable(self):
        # Never.
        return False

    ### Overridden from Constable.

    def canCoerceConstantFrom(self, const, srctype):
        return isinstance(srctype, HiltiType)

    def coerceConstantFrom(self, cg, const, srctype):
        return const


@hilti(None, -1)
class Unknown(Type):
    """Place-holder type when we do not yet know the actual type of something.

    All unknown types should eventually be replaced by ~~resolve method. If
    any is left after resolving, compilation will be aborted by this type's
    ~~validate method.

    name: string or None - If the unknown type is refered to by an identifier
    (i.e., a forward reference), its name. None otherwise.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, name=None, location=None):
        super(Unknown, self).__init__(location)
        Unknown.setTypeName("<unknown type>")
        self._id = name

    def idName(self):
        """Returns the identifier name that needs to be resolved to replace
        this unknown type.

        Returns: string or None - The name of the identifier, or None if the
        type does not depend on an identifier."""
        return self._id

    ### Overridden from Type.

    def name(self):
        # Need to return a unique name here so that multiple instances of
        # Unknown are considered to be separate types.
        return "unknown_type_%s" % builtin_id(self)

    def cmpWithSameType(self, other):
        # Never match another unknown.
        return False

    def _validate(self, vld):
        vld.error(self, "unknown identifier %s" % self._id)

    def _resolve(self, resolver):
        i = resolver.scope().lookup(self._id)
        if isinstance(i, id.Type):
            return i.type()

        return self

    # We provide the following as dummies to avoid needing to check for the
    # presence of this type in many of the validate() methods. They return
    # True because the unknown type will be reported independently.

    def canCoerceConstantTo(self, const, dsttype):
        return True

    def canCoerceTo(self, dsttype):
        return True

@hilti(None, -1)
class MetaType(ValueType):
    """Type representing a type.

    location: ~~Location - The location where the type was defined.
    """
    def __init__(self, location=None):
        super(MetaType, self).__init__(location)
        MetaType.setTypeName("<meta type>")

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeTypeInfoPtr()

    def canCoerceFrom(self, srctype):
        return isinstance(srctype, HiltiType)

    def llvmCoerceFrom(self, cg, value, srctype):
        assert self.canCoerceFrom(srctype)
        return cg.llvmTypeInfoPtr(srctype)

    ### Overridden from ValueType.

    def instantiable(self):
        # Never.
        return False

def _matchWithTypeClass(t, cls):
    """Checks whether a type instance matches with a type class; cls can be a
    tuple of classes as well."""

    # Special case for tuples/lists of classes, for which a match with any
    # of the classes in there is fine.
    if builtin_type(cls) == types.ListType or builtin_type(cls) == types.TupleType:
        for cl in cls:
            if cl and _matchWithTypeClass(t, cl):
                return True

        return False

    # Special case for the Any class, which matches any other type.
    if cls == Any:
        return True

    # Standard case, need match with the cls's type hierarchy.
    return isinstance(t, cls)

def canCoerceTo(src, dst):
    """Checks whether a type can be coerced into another.

    src: ~~Type - The source type.

    dst: ~~Type - The destination type into which *src* should be coerced.

    Returns: bool - True if the coercion is possible.

    Note: This is the primary method to check whether a type can be coerced.
    Do not call ~~type.canCoerceTo directly.
    """
    if isinstance(dst, Any):
        return True

    if src.canCoerceTo(dst):
        return True

    return dst.canCoerceFrom(src)

def llvmCoerceTo(cg, val, src, dst):
    """Coerce a value of one type into a value of another. This function must
    be only called if ~~canCoerceTo indicates that coercion is possible.

    cg: ~~CodeGen - The current code generator.

    val: llvm.core.Value - The value to be coerced.

    src: ~~Type - The source type.

    dst: ~~Type - The destination type into which *src* should be coerced.

    Returns: llvm.core.Value - The coerced value.

    Note: This is the primary method to coerce a value.  Do not call
    ~~type.llvmCoerceTo directly.
    """
    if isinstance(dst, Any):
        return val

    if src.canCoerceTo(dst):
        return src.llvmCoerceTo(cg, val, dst)

    if dst.canCoerceFrom(src):
        return src.llvmCoerceFrom(cg, val, src)

    util.internal_error("cannot coerce value of type %s to type %s" % (src, dst))

def canCoerceConstantTo(const, dst):
    """Checks whether a constant of a type can be coerced into a constant of
    another type.

    const: ~~Constant - The constant to be coerced.

    dst: ~~Type - The destination type into which *const* should be coerced.

    Returns: bool - True if the coercion is possible.

    Note: This is the primary method to check whether a constant can be
    coerced. Do not call ~~type.canCoerceConstantTo directly.
    """
    if isinstance(dst, Any):
        return True

    if const.type().canCoerceConstantTo(const, dst):
        return True

    return dst.canCoerceConstantFrom(const, const.type())

def coerceConstantTo(cg, const, dst):
    """Coerce a constant of one type into a constant of another. This function
    must be only called if ~~canCoerceConstantTo indicates that coercion is
    possible.

    cg: ~~CodeGen - The current code generator.

    const: ~~Constant - The constant to be coerced.

    dst: ~~Type - The destination type into which *src* should be coerced.

    Returns: ~~Constant - The coerced constant

    Note: This is the primary method to coerce a constant.  Do not call
    ~~type.coerceConstantTo directly.
    """
    if isinstance(dst, Any):
        return const

    if const.type().canCoerceConstantTo(const, dst):
        return const.type().coerceConstantTo(cg, const, dst)

    if dst.canCoerceConstantFrom(const, const.type()):
        return dst.coerceConstantFrom(cg, const, const.type())

    util.internal_error("cannot coerce value of type %s to type %s" % (const.type(), dst))




