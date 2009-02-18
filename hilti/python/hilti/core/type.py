# $Id$

builtin_type = type

import types 
import inspect

import util

### Base classes

class Type(object):
    """The base class for all data types provided by the HILTI language. 
    *name* is a string with a name of the type in a readable form, used in
    error messages and debugging output. *docname* is an optional string which
    is used instead of *name* in the automatically generated instruction
    documentation; it is not used anywhere else.
    
    Throughout the HILTI compiler, type compatibility is checked with the +==+
    operator of Type objects. A comparision of the form +t == other+, where
    *t* is an instance of Type (including any derived class), is performed in
    the following way:
    
    * If *other* is likewise an instance of Type, the two types match
      according to the following rules:
    
      1. If the two types are instances of the same class (exact match; *not*
         including any common ancestors), the match succeeds if the
         type-specific :meth:`cmpWithSameType` method returns *True*. The
         default implementation of this method in Type compares the results of
         two object's :class:`name` method. 
         
      2. If either of the two types is an instance of ~Any, the match succeeds.
      
      3. If neither (1) or (2) is the case, the match fails. 
      
    * If *other* is a *class* that is derived from Type, then the match
      succeeds if and only if *t* is an instance of any class derived from
      *other*. 
      
    * If *other* is a tuple or list, then *t* is recursively compared to any
      elements contained therein, and the match succeeds if at least one
      element matches with *t*. 

    * In all other cases, the comparision yields *False*.
    
    Any class derived from Type must include a class-variable +_name+
    containing a string that is suitable for using in error messages to
    describes the HILTI type that the class is representing. 
    """
    def __init__(self, name, docname=None):
        self._name = name
        self._docname = docname if docname else name
    
    def name(self):
        """Returns a string with the name of the type in a readable form."""
        return self._name
    
    def docname(self):
        """Returns a string with the name of the type suitable for use in the
        automatically generated instruction documentation."""
        return self._docname

    def cmpWithSameType(self, other):
        """Compares object with another one of the *same* class. Returns True
        if they are compatible. This method can be overriden by derived
        classes; the default implementation compares the results of two
        :meth:`name` methods.""" 
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

class HiltiType(Type):
    """Base class for all types which can be directly instantiated by the user
    in a HILTI program, either in global/local variable or on the heap. 
    *args* is list of type parameters; for non-parametrized types it will be
    the empty list. *name* and *docname* are the same as for ~~Type.
    
    If a derived class detects an error with any of the arguments, it must
    raise an ~~ParameterMismatch exception."""
    def __init__(self, args, name, docname=None):
        if args:
            name = "%s<%s>" % (name, ",".join([str(arg) for arg in args]))
        
        super(HiltiType, self).__init__(name, docname=docname)

    _name = "HILTI type"
        
    class ParameterMismatch(Exception):
        """Exception class to indicate a problem with type parameter. *param*
        is the parameter which caused the trouble, and *reason* is a string
        that explains what's wrong with it."""
        def __init__(param, reason):
            self._param = param
            self._reason = reason
            
        def __str__(self):
            return "error in type parameter: %s (%s)" % (self._reason, self._param)

        
class StorageType(HiltiType):
    """Base class for all types that can be directly stored in a HILTI global
    or local variable (as opposed types derived from ~~HeapType, which must be
    allocated on the heap and can only be accessed via a ~~Reference). Types
    derived from StorateType cannot be allocated on the heap. The arguments
    are the same as for ~~HiltiType.
    """
    def __init__(self, args, name, docname=None):
        super(StorageType, self).__init__(args, name, docname=docname)

    _name = "storage type"
        
class HeapType(HiltiType):
    """Base class for all types that must be allocated on the heap and can
    only be accessed via a ~~Reference (as opposed to types derived from
    ~~StorageType, which can be directly stored in a HILTI global or local
    variable). The arguments are the same as for ~~HiltiType.
    """
    def __init__(self, args, name, docname=None):
        super(HeapType, self).__init__(args, name, docname=docname)

    _name = "heap type"
    
class OperandType(Type):
    """Base class for all types that can only be as operands or function
    arguments but not stored in a variable or on the heap. The arguments are
    the same as for ~~Type.
    """
    def __init__(self, name, docname=None):
        super(OperandType, self).__init__(name, docname=docname)
        
    _name = "operand type"
    
class TypeDeclType(Type):
    """Base class for types that represent a custom, user-declared HILTI type.
    *t* is the declared type. *name* and *docname* are the same as for ~~Type.
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
    """A type for strings."""
    def __init__(self):
        super(String, self).__init__([], String._name)
        
    _name = "string"
        
class Integer(StorageType):
    """A type for integers.  *args* must be a list containing a single
    integer, specifying the bit-width of values represented by this type. The
    width can be zero to match any other integer types independent of their
    width. Note that the HILTI language does not allow to *use* integers of
    width zero however."""
    def __init__(self, args):
        
        if type(args) == types.IntType:
            args = [args]
        
        super(Integer, self).__init__(args, Integer._name)
        
        assert len(args) == 1
        
        try:
            self._width = int(args[0])
        except ValueError:
            raise ParameterMismatch(args[0], "cannot convert to integer")
            
    def width(self):
        """Returns the bit-width of integers represented by this type. If the
        returned width is zero, this type matches all other integer types
        independent of their width."""
        return self._width

    def setWidth(self, width):
        """Set the bit-width of integers represented by this type to the
        integer *width*."""
        self._width = width
    
    def cmpWithSameType(self, other):
        if self._width == 0 or other._width == 0:
            return True
        
        return self._width == other._width
    
    _name = "int"
    
class Bool(StorageType):
    """A type for boolean values."""
    def __init__(self):
        super(Bool, self).__init__([], Bool._name)
        
    _name = "bool"
    
class Struct(StorageType):
    """The Type for structs. *fields* is list of ~~ID objects defining the fields of
    the struct."""
    def __init__(self, fields):
        name = "struct { %s }" % ", ".join(["%s %s" % (id.name(), id.type().name()) for id in fields])
        super(Struct, self).__init__([], name)
        self._ids = fields
    
    def Fields(self):
        """Returns the list of ~~ID objects defining the fields of the
        struct."""
        return self._ids

    _name = "struct"
    
class StructDecl(TypeDeclType):
    """The Type for the declration of custom struct types. *structt* is the
    ~~Struct which is declared."""
    def __init__(self, structt):
        super(StructDecl, self).__init__(structt, "<struct decl> %s" % structt.name())
        
    _name = "struct declaration"
    
class Function(Type):
    """A type for functions. *args* is a list of ~~ID objects representing the
    function's arguments. *resultt* is the result ~~Type of the function.""" 
    def __init__(self, args, resultt):
        name = "(function (%s) -> %s)" % (", ".join([str(id.type()) for id in args]), resultt)
        super(Function, self).__init__(name)
        self._ids = args
        self._result = resultt
        
    def Args(self):
        """Returns the list of ~~ID objects representing the function's
        arguments."""
        return self._ids
    
    def getArg(self, name):
        """Returns the ~~ID corresponding to the given argument *name*, or
        *None* if there is no such argument."""
        for id in self._ids:
            if id.name() == name:
                return id
            
        return None
    
    def resultType(self):
        """Returns the function's result ~~Type."""
        return self._result

    _name = "function"

class Label(OperandType):
    """A type for block labels."""
    def __init__(self):
        super(Label, self).__init__("label")

    _name = "label"
    
class Void(OperandType):
    """A type used to indicate that a ~~Function does not return any value."""
    def __init__(self):
        super(Void, self).__init__("void")
        
    _name = "void"
    
class Any(OperandType):
    """A wildcard type that matches any other ~~Type."""
    def __init__(self, name):
        super(Any, self).__init__("any")

    _name = "any"

class Unknown(OperandType):
    """A place-hilder type to indicate that we don't know the type yet. Used
    during parsing"""
    def __init__(self):
        super(Unknown, self).__init__("unknown")

    _name = "unknown"
    
class Tuple(OperandType):
    """A type to represent a tuple of values. *types* is a list of ~~Type
    objects indicated the types of the tuple members."""
    
    def __init__(self, types):
        super(Tuple, self).__init__("(%s)" % ", ".join([t.name() for t in types]))
        self._types = types
        self._matches_any = False

    _name = "tuple"

def Optional(t):
    """Returns an object that can be used as an ~~instruction's operand type
    to indicate that the operand is optional. *t* is the ~~Type the operand
    must have if it's there.
    
    Note: This is just a short-cut for the tuple +(None, t)+.
    """
    return (None, t)

def isOptional(t):
    """Returns *True* if the given ~~Type *type* represents an optional type as
    returned by ~~Optional."""
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
    """Returns a readable representation of the class *cls*; can be a tuple of
    classes."""

    if cls == None:
        return "none"
    
    if builtin_type(cls) == types.ListType or builtin_type(cls) == types.TupleType:
        if not doc:
            return " or ".join([fmtTypeClass(t) for t in cls])
        else:
            return "|".join([fmtTypeClass(t) for t in cls])
    
    return cls._name
    
# List of keywords that the parser will understand to instantiate types.  Each
# keyword is mapped to a tuple (class, args, defaults) where "class" is the name
# of the class to instantiate for this keyword; num_args is the number of type
# parameters this type expects; and default is an optional list with predefined
# parameters to be used *instead* of user-supplied params (optional). All
# classes given here must be derived from HiltiType.

_keywords = {
	"int": (Integer, 1, None),
	"int8": (Integer, 1, [8]),
	"int16": (Integer, 1, [16]),
	"int32": (Integer, 1, [32]),
	"int64": (Integer, 1, [64]),
    "string": (String, 0, None),
    "bool": (Bool, 0, None),
    }

def getHiltiType(name, args):
    """Instantiates a ~~HiltiType, based on the given name and type
    parameters. Returns a tuple *(success, result)*. If *success* is *True*,
    the type was successfully created and result will be the corresponding
    ~~HiltiType. If *success* is *False*, there was an error and *result* will
    contain a string with an error message."""
    
    try:
        (cls, numargs, defs) = _keywords[name]
    except KeyError:
        return (False, "no such type, %s" % name)
    
    if args and (numargs == 0 or defs != None):
        return (False, "type %s does not accept type paramters" % name)
    
    if (args and len(args) != numargs) or (not args and not defs and numargs > 0):
        return (False, "wrong number of parameters for type %s (expected %d, have %d)" % (name, numargs, len(args)))
    
    if defs:
        args = defs
        
    try:
        if args:
            return (True, cls(args))
        else:
            return (True, cls())
        
    except HiltiType.ParameterMismatch, e:
        return (False, str(e))

def getHiltiTypeNames():
    """Returns a list of names of all ~HiltiType types that can directly be
    instantiated in a HILTI program."""
    return _keywords.keys()
    
    
