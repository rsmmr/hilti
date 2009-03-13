# $Id$

builtin_type = type

import types 
import inspect

import util

### Base classes

class Type(object):
    """Base class for all data types provided by the HILTI language.  
    
    name: string - The name of type in a readable form suited to present to
    the user (e.g., in error messages and debugging output).
        
    docname: streing - Optional string which, if given, will be used in the
    automatically generated instruction documentation instead of *name*; it is
    not used anywhere else than in the documentation.

    Note:
    
    Throughout the HILTI compiler, type compatibility is checked by applying
    Python's ``==`` operator to Type-derived objects. A comparision of the
    form ``t == other``, with *t* being an instance of Type or any derived
    class, is performed according to these rules:
    
    * If *other* is likewise an instance of Type, the two types match
      according to the following conditions:
    
      1. If the two types are instances of the same class (exact match; *not*
         considering any common ancestors), the match succeeds if the
         type-specific :meth:`cmpWithSameType` method returns True. The
         default implementation of this method as defined in Type compares the
         results of two objects :class:`name` methods. 
         
      2. If either of the two types is an instance of ~~Any, the match succeeds.

      3. If neither (1) or (2) is the case, the match fails. 
      
    * If *other* is a *class* that is derived from Type, then the match
      succeeds if and only if *t* is an instance of any class derived from
      *other*. 
      
    * If *other* is a tuple or list, then *t* is recursively compared to any
      elements contained therein, and the match succeeds if at least one
      element matches with *t*. 

    * In all other cases, the comparision yields False.
      
    Any class derived from Type must include a class-variable ``_name``
    containing a string that is suitable for use in error messages to
    describes the HILTI type that the class is representing. In addition, all
    Type-derived classes that can be instantiated directly (vs. base classes)
    must have another class-variable *_id* with an integer that is unique
    across all of them; the integer must have a matching constant 
    ``__HLT_TYPE_*`` defined in :download:`/libhilti/hilti_intern.h`. 
    
    Todo: Perhaps we should autogenerate the *_id* constants in some way and
    also automatically synchronize them with ``hilti_intern.h``. I'm however
    not quite sure what's an elegant way to do that. 
    """
    def __init__(self, name, docname=None):
        self._name = name
        self._docname = docname if docname else name
    
    def name(self):
        """Returns the name of the type.
        
        Returns: string - The name of the type.
        """
        return self._name
    
    def docname(self):
        """Returns the name of the type as used in the instruction
        documentation.
        
        Returns: string - The documentation name of the type.
        """
        return self._docname

    def cmpWithSameType(self, other):
        """Compares with another objects of the same class. The default
        implementation compares the results of the two instances'
        :meth:`name`` methods. Derived classes can override the method 
        to implement their own type checking.
        
        Returns: bool - True if the two objects are compatible. 
        """
        assert self.__class__ == other.__class__
        return self.name() == other.name()
    
    def __str__(self):
        return self.name()
    
    def __eq__(self, other):
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
        # match. 
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
    
    _name = "type"
    _id = 0 # Zero is used as an error indicator.


class HiltiType(Type):
    """Base class for all HILTI types that can be directly instantiated in a
    HILTI program. That includes use in global and local variables, as well as
    allocation on the heap. 
    
    During run-time, libhilti will provide a type-info objects for all
    HiltiType instances.
    
    args: list of any - Type parameters, or the empty list for
    non-parameterized types.  If a derived class detects an error with any of
    the arguments, it must raise a ~~ParameterMismatch exception.
    
    wildcard: bool - True if the type's parameters contain a wildcard so that
    it can match multiple other types.
    
    name: string - Same as for :meth:`~hilti.core.type.type`. 
    docname: string - Same as for :meth:`~hilti.core.type.type`.
    
    Raises: ~~ParameterMismatch - Raised when there's a problem with one of
    the type parameters. The error should not have been reported to the user
    already; an error message will be constructed from the exeception's
    information.
    
    Note: For HiltiTypes, it is important that the results of their ~~name
    methods can be used to identify the types, including any potential
    parameters. In particular, if two HiltiTypes share the same name, they are
    considered equivalent, and likewise HiltiTypes with different names are
    considered non-equivalent. This is achieved automatically though by
    appending a readable representation of all parameters to the type's base
    name passed to the ctor via *name*.
    """
    def __init__(self, args, name, wildcard=False, docname=None):
        if args:
            name = "%s<%s>" % (name, ",".join([str(arg) for arg in args]))
        
        super(HiltiType, self).__init__(name, docname=docname)
        self._args = args
        self._wildcard = wildcard

    _name = "HILTI type"

    def args(self):
        """Returns the type's parameters.
        
        args: list of any - The type parameters, or the empty list for
        non-parameterized types.  
        """
        return self._args

    def wildcardType(self):
        """Returns whether this type can match multiple other types.
        That is considered to be the case when any of the arguments
        is ``*``.
        
        Returns: bool - True if a wildcard type.
        """
        return self._wildcard

    class ParameterMismatch(Exception):
        """Exception class to indicate a problem with a type parameter.
        
        param: any - The parameter which caused the trouble.
        reason: string - A string explaining what went wrong.
        """
        def __init__(self, param, reason):
            self._param = param
            self._reason = reason
            
        def __str__(self):
            return "error in type parameter: %s (%s)" % (self._reason, self._param)

        
class StorageType(HiltiType):
    """Base class for all types that can be directly stored in a HILTI
    variable. Types derived from StorageType cannot be allocated on the heap.
    
    The arguments are the same as for ~~HiltiType.
    """
    def __init__(self, args, name, wildcard=False, docname=None):
        super(StorageType, self).__init__(args, name, wildcard=wildcard, docname=docname)

    _name = "storage type"
        
class HeapType(HiltiType):
    """Base class for all types that must be allocated on the heap. Types
    derived from HeapType cannot be stored directly in variables and only
    accessed via a ~~Reference. 
    
    The arguments are the same as for ~~HiltiType.
    """
    def __init__(self, args, name, wildcard=False, docname=None):
        super(HeapType, self).__init__(args, name, wildcard=wildcard, docname=docname)

    _name = "heap type"
    
class OperandType(Type):
    """Base class for all types that can only be used as operands and function
    arguments. These types cannot be stored in variables nor on the heap. 
    
    The arguments are the same as for ~~Type.
    """
    def __init__(self, name, docname=None):
        super(OperandType, self).__init__(name, docname=docname)
        
    _name = "operand type"
    
class TypeDeclType(Type):
    """Base class for types that represent a custom user-declare HILTI type.
    
    t: ~~Type - The declared type.
    name: string - Same as for :meth:`~hilti.core.type.type`.
    docname: string - Same as for :meth:`~hilti.core.type.type`.
    """
    def __init__(self, t, name, docname=None):
        super(TypeDeclType, self).__init__(name, docname)
        self._type = type
    
    def type(self):
        """Return the user-declared type."""
        return self._type

    _name = "type-declaration type"

# Actual types.    

class String(StorageType):
    """Type for strings."""
    def __init__(self):
        super(String, self).__init__([], String._name)
        
    _name = "string"
    _id = 3
        
class Integer(StorageType):
    """Type for integers.  
    
    args: int, or a list containing a single int or a "*" - The
    integer specifies the bit-width of integers represented by this
    type. The width can be "*" to match any other integer type
    independent of its widget. Note however that the HILTI language
    does not allow to create instances of integers with no specified
    width.
    """
    def __init__(self, args):
        
        if type(args) == types.IntType or type(args) == types.StringType:
            args = [args]

        assert len(args) == 1
        
        if args[0] == "*":
            self._width = 0
            wildcard = True
            args = [0]
        else:
            try:
                self._width = int(args[0])
            except ValueError:
                raise HiltiType.ParameterMismatch(args[0], "cannot convert to integer")
            
            if self._width <= 0 or self._width > 64:
                raise HiltiType.ParameterMismatch(args[0], "integer width must be between 1 and 64 bits")
            
            wildcard = False
            
        super(Integer, self).__init__(args, Integer._name, wildcard=wildcard)

    def width(self):
        """Returns the bit-width of the type's integers.
        
        Returns: int - The number of bits available to represent integers of
        this type. If the returned width is zero, the type matches all other
        integer types. 
        """
        return self._width

    def setWidth(self, width):
        """Sets the bit-width of the type's integers.
        
        width: int - The new bit-width. 
        """
        self._width = width
    
    def cmpWithSameType(self, other):
        if self._width == 0 or other._width == 0:
            return True
        
        return self._width == other._width
    
    _name = "int"
    _id = 1
    
class Double(StorageType):
    """Type for doubles."""
    def __init__(self):
        super(Double, self).__init__([], Double._name)
        
    _name = "double"
    _id = 2

class Bool(StorageType):
    """Type for booleans."""
    def __init__(self):
        super(Bool, self).__init__([], Bool._name)
        
    _name = "bool"
    _id = 3

class Tuple(StorageType):
    """A type for tuples of values. 
    
    types: list of ~~Type, or a list ["*"] - The types of the
    individual tuple elements. A list of just "*" is treated as
    wildcard type matching any other tuple. Note however that the
    HILTI language does not allow to create instances of such
    wildcard tuples.
    """
    def __init__(self, types):
        if "*" in types:
            if len(types) != 1:
                raise HiltiType.ParameterMismatch(args[1], "cannot mix types with '*'")
            
            self._types = []
            self._any = True
            
        else:
            self._types = types
            self._any = False

        super(Tuple, self).__init__(self._types, Tuple._name, wildcard=self._any)
            
    def types(self):
        """Returns the types of the typle elements.
        
        Returns: list of ~~StorageType - The types. The list will be empty for
        ``tuple<*>``.
        """
        return self._types
        
    def cmpWithSameType(self, other):
        if self._any or other._any:
            return True
        
        return self.types() == other.types()
        
    _name = "tuple"
    _id = 5

class Struct(HeapType):
    """Type for structs. 
    
    fields: list of ~~ID - The IDs defined the fields of the struct.
    """
    def __init__(self, fields):
        name = "struct { %s }" % ", ".join(["%s %s" % (id.name(), id.type().name()) for id in fields])
        super(Struct, self).__init__([], name)
        self._ids = fields
    
    def Fields(self):
        """Returns the struct's fields.
        
        Returns: list of ~~ID - The struct's fields.
        """
        return self._ids

    _name = "struct"
    
class StructDecl(TypeDeclType):
    """Type for struct declarations. 
    
    structt: ~~Struct - The struct type declared.
    """
    def __init__(self, structt):
        super(StructDecl, self).__init__(structt, "%s" % structt.name())
        
    _name = "struct declaration"
    
class Function(Type):
    """Type for functions. 
    
    args: list of (~~ID, default) tuples - The function's arguments, with
    *default* being optional default values. Set *default* to *None* if no
    default.
    
    resultt - ~~Type - The type of the function's return value (~~Void for none). 
    """
    def __init__(self, args, resultt):
        name = "function (%s) -> %s" % (", ".join([str(id.type()) for (id, default) in args]), resultt)
        super(Function, self).__init__(name)
        self._ids = [id for (id, default) in args]
        self._defaults = [default for (id, default) in args]
        self._result = resultt
        
    def args(self):
        """Returns the functions's arguments.
        
        Returns: list of ~~ID - The function's arguments. 
        """
        return self._ids

    def argsWithDefaults(self):
        """Returns the functions's arguments with any default values.
        
        Returns: list of (~~ID, default) - The function's arguments with their
        default values. If there's no default for an arguments, *default*
        will be None.
        """
        return zip(self._ids, self._defaults)
    
    def getArg(self, name):
        """Returns the named function argument.
        
        Returns: ~~ID - The function's argument with the given name, or None
        if there is no such arguments. 
        """
        for id in self._ids:
            if id.name() == name:
                return id
            
        return None
    
    def resultType(self):
        """Returns the functions's result type.
        
        Returns: ~~Type - The function's result. 
        """
        return self._result

    _name = "function"

class Label(OperandType):
    """Type for block labels."""
    def __init__(self):
        super(Label, self).__init__("label")

    _name = "label"
    
class Void(OperandType):
    """Type representing a non-existing function result."""
    def __init__(self):
        super(Void, self).__init__("void")
        
    _name = "void"
    
class Any(OperandType):
    """Wildcard type that matches any other type."""
    def __init__(self):
        super(Any, self).__init__("any")

    _name = "any"

class Unknown(OperandType):
    """Place-holder type when the real type is unknown. This type is used
    during parsing when the final types have not been determined yet."""
    def __init__(self):
        super(Unknown, self).__init__("unknown")

    _name = "unknown"
    
def Optional(t):
    """Returns a type representing an optional operand. The returns
    object can be used with an ~~Instruction operand to indicate
    that it should be either of type *t* or not given at all.
    
    t: ~~Type *class* - The type the operand must have if it's specified.
    
    Note: This is just a short-cute for using +(None, t)+.
    """
    return (None, t)

def isOptional(t):
    """Checks whether an operand is optional. To be recognized as optional,
    *t* must be of the form returned by ~~Optional.
    
    t: class derived from ~~Type - The type to be tested. 
    
    Returns: boolean - True if *t* is optional as returned by ~~Optional.
    """
    if type(t) != types.TupleType and type(t) != types.ListType:
        return False
    
    return None in t

def _matchWithTypeClass(t, cls):
    """Checks whether a type instance matches with a type class; cls can be a tuple
    of classes as well."""

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

def fmtTypeClass(cls, doc=False):
    """Returns a readable representation of a type class.
    
    cls: class derived from ~~Type, or a list/tuple of such classes - The type
    to be converted into a string; if a list or tuple is given, all elements
    are converted individually and merged in tuple-syntax.
    
    doc: boolean - If true, the string is formatted for inclusion into the
    automatically generated documentation; the format then will be slightly
    different.
    
    Returns: string - The readable representation.
    """
    if cls == None:
        return "none"
    
    if builtin_type(cls) == types.ListType or builtin_type(cls) == types.TupleType:
        if not doc:
            return " or ".join([fmtTypeClass(t) for t in cls])
        else:
            return "|".join([fmtTypeClass(t) for t in cls])
    
    return cls._name
    
# List of keywords that the parser will understand to instantiate types.  Each
# keyword is mapped to a tuple (class, args, defaults) where "class" is the
# name of the class to instantiate for this keyword; num_args is the number
# of type parameters this type expects (-1 for variable but one minimum); and
# default is an optional list with predefined parameters to be used *instead* of
# user-supplied params (optional). All classes given here must be derived from
# HiltiType.

_keywords = {
	"int": (Integer, 1, None),
	"int8": (Integer, 1, [8]),
	"int16": (Integer, 1, [16]),
	"int32": (Integer, 1, [32]),
	"int64": (Integer, 1, [64]),
	"double": (Double, 0, None),
    "string": (String, 0, None),
    "bool": (Bool, 0, None),
    "tuple": (Tuple, -1, None),
    }

_all_hilti_types = {}
    
def getHiltiType(name, args = []):
    """Instantiates a ~~HiltiType from a type name. 
    
    *name*: string - The name of type as used in HILTI programs. 
    *args*: list of any - A list of type parameters if the type is
    parametrized; the empty list for non-parameterized types.
    
    Returns: tuple (success, result) - If *success* is True, the type was
    successfully instantiated and *result* will be the newly created type
    object. If *success* is False, there was an error and *result* will be a
    string containing an error message."""
    try:
        (cls, num_args, defs) = _keywords[name]
    except KeyError:
        return (False, "no such type, %s" % name)
    
    if args and (num_args == 0 or defs != None):
        return (False, "type %s does not accept type paramters" % name)
    
    wrong_args = False
    
    if args and num_args >= 0 and len(args) != num_args:
        wrong_args = True
    
    if num_args == -1 and not args:
        wrong_args = True
        
    if  not args and not defs and num_args > 0:
        wrong_args = True

    if wrong_args:
        return (False, "wrong number of parameters for type %s (expected %d, have %d)" % (name, num_args, len(args)))
    
    if defs:
        args = defs
        
    try:
        if args:
            t = cls(args)
        else:
            t = cls()
            
        try:
            return (True, _all_hilti_types[t.name()])
        except KeyError:
            _all_hilti_types[t.name()] = t
            return (True, t)
        
    except HiltiType.ParameterMismatch, e:
        return (False, str(e))

def getHiltiTypeNames():
    """Returns a list of HILTI types. The list contains the names of all types
    that correspond to a ~~HiltiType-derived class. The names can be used with
    :meth:`getHiltiType` to instantiate a corresponding type object.
    
    Returns: list of strings - The names of the types.
    """
    return _keywords.keys()
    
def getAllHiltiTypes():
    """Returns a list of all HILTI types. The list contains all instances of
    ~~HiltiType that have been created so far.
    
    Returns: list ~~HiltiType - The complete set of HILTI types.
    """
    return _all_hilti_types.values()
