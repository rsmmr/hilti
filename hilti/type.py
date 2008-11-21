# $Id$
#
# Types provided by BIR.

import types 
import util

class Type(object):
    def __init__(self, name ="<no-name>", docname=None):
        self._name = name
        self._docname = docname if docname else name
    
    def name(self):
        return self._name
    
    def docname(self):
        return self._name
    
    def __str__(self):
        return self.name()
    
    def __eq__(self, other):
        # FIXME: All types overiding __eq__ must do these checks too. Is there a nice way to achieve that?
        if isinstance(other, OneOf):
            return OneOf.__eq__(other, self)
        
        if id(other) == id(Any):
            return True
      
        return self._name == other._name

###
    
class StorageType(Type):
    def __init__(self, name):
        super(StorageType, self).__init__(name)
        
class AtomicType(StorageType):
    def __init__(self, name):
        super(AtomicType, self).__init__(name)

class CompositeType(StorageType):
    def __init__(self, name):
        super(CompositeType, self).__init__(name)
        
class StringType(AtomicType):
    def __init__(self, name):
        super(StringType, self).__init__(name)
        
class IntegerType(AtomicType):
    def __init__(self, name):
        super(IntegerType, self).__init__(name)

class BoolType(AtomicType):
    def __init__(self, name):
        super(BoolType, self).__init__(name)
        
class StructType(StorageType):
    def __init__(self, ids):
        super(StructType, self).__init__(name)
        self._ids = ids
        
    def name(self):
        return "<struct-type-fix-me>"
    
    def IDs(self):
        return self._ids

    def __eq__(self, other):
        return self.name() == other.name() and self._ids == other._ids
    
###
        
class TypeDeclType(Type):
    def __init__(self, type):
        super(TypeDeclType, self).__init__()
        self._type = type
    
    def type(self):
        return self._type

    def __eq__(self, other):
        return self._type == other._type
    
class StructDeclType(TypeDeclType):
    def __init__(self, structtype):
        super(StructDeclType, self).__init__(structtype)
        
###
        
class FunctionType(Type):
    def __init__(self, ids = None, result_type = None, generic=False):
        super(FunctionType, self).__init__("function")
        self._ids = ids
        self._result = result_type
        self._generic = generic
        
    def IDs(self):
        return self._ids
    
    def resultType(self):
        return self._result

    def __eq__(self, other):
        
        if not isinstance(other, FunctionType):
            return

        # The generic function matches all other functions
        if self._generic or other._generic:
            return True
        
        def idtypes(ids):
            if not ids:
                return []
            return [id.type() for id in ids]
        
        return idtypes(self._ids) == idtypes(other._ids) and self._result == other._result

    def __str__(self):
        if self._generic:
            return "[(*)->*]" 
        
        else:
            return "[(%s)->%s]" % (", ".join([str(id.type()) for id in self._ids]), self._result)
    
###

class LabelType(Type):
    def __init__(self, name):
        super(LabelType, self).__init__(name)

class VoidType(Type):
    def __init__(self, name):
        super(VoidType, self).__init__(name)
        
class AnyType(Type):
    def __init__(self, name):
        super(AnyType, self).__init__(name)

    def __eq__(self, other):
        return True
        
class TupleType(Type):
    def __init__(self, types, generic=False):
        super(TupleType, self).__init__("(%s)" % ", ".join([t.name() for t in types]))
        self._types = types
        self._generic = generic

    def __eq__(self, other):
        if isinstance(other, TupleType):
            # Generic tuple matches all other tuples
            if self._generic or other._generic:
                return True
            
        return self._types == other._types
        
###

class OneOf(Type):
    def __init__(self, types):
        name = "[%s]" % "|".join([str(t) for t in types])
        docname = " or ".join(["*%s*" % str(t) for t in types])
        
        super(OneOf, self).__init__(name, docname=docname)
        self._types = types
        
    def __eq__(self, other):
        if isinstance(other, OneOf):
            return self._types == other._types
        
        return other in self._types

Int16 = IntegerType("int16")        
Int32 = IntegerType("int32")        
Integer = OneOf((Int16, Int32))

String = StringType("string")
Bool = BoolType("bool")
Label = LabelType("label")
Void = VoidType("void")
Any = AnyType("any")
AnyTuple = TupleType((), generic=True)
Function = FunctionType(generic=True)

# Returns a readable respresentation of the given type (which might be a tuple of types)
def name(t, docstring = False):
    # FIXME: Get rid fo this.
    
    if not docstring:
        return t.name()
    else:
        return t.docname()

_types = {
	"int16": Int16,
    "int32": Int32,
    "string": String,
    "bool": Bool,
    "void": Void,
    }

# Turns a type name as specificed in the input code into an actual type. 
def type(name):
    try:
        return _types[name]
    except KeyError:
        return None

# Returns a list of all type names.
def typenames():
    return _types.keys()

    
    
    
