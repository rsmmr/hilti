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
        return self._docname
    
    def __str__(self):
        return self.name()
    
    def _eq_common(self, other):
        if isinstance(other, OneOf):
            return (True, OneOf.__eq__(other, self))
        
        if id(other) == id(Any):
            return (True, True)
    
        if self.__class__ != other.__class__:
            return (True, False)
        
        return (False, False)
        
    def __eq__(self, other):
        (done, result) = self._eq_common(other)
        if done:
            return result
        
        return self._name == other._name

###
    
class StorageType(Type):
    def __init__(self, name, docname=None):
        super(StorageType, self).__init__(name, docname=docname)
        
class AtomicType(StorageType):
    def __init__(self, name, docname=None):
        super(AtomicType, self).__init__(name, docname=docname)

class CompositeType(StorageType):
    def __init__(self, name):
        super(CompositeType, self).__init__(name)
        
class StringType(AtomicType):
    def __init__(self, name):
        super(StringType, self).__init__(name)
        
class IntegerType(AtomicType):
    # 0 means "any"
    def __init__(self, width = 0):
        if width > 0:
            super(IntegerType, self).__init__("int:%d" % width)
        else: 
            super(IntegerType, self).__init__("int:*", docname="int")
            
        self._width = width
        
    def width(self):
        return self._width

    def __eq__(self, other):
        (done, result) = self._eq_common(other)
        if done:
            return result

        if self._width == 0 or other._width == 0:
            return True
        
        return self._width == other._width
    
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
        (done, result) = self._eq_common(other)
        if done:
            return result
        
        return self.name() == other.name() and self._ids == other._ids
    
###
        
class TypeDeclType(Type):
    def __init__(self, type):
        super(TypeDeclType, self).__init__()
        self._type = type
    
    def type(self):
        return self._type

    def __eq__(self, other):
        (done, result) = self._eq_common(other)
        if done:
            return result
        
        return self._type == other._type
    
class StructDeclType(TypeDeclType):
    def __init__(self, structtype):
        super(StructDeclType, self).__init__(structtype)
        
###
        
class FunctionType(Type):
    def __init__(self, ids = [], result_type = None, generic=False):
        super(FunctionType, self).__init__("function")
        self._ids = ids
        self._result = result_type
        self._generic = generic
        
    def IDs(self):
        return self._ids
    
    def getID(self, name):
        for id in self._ids:
            if id.name() == name:
                return id
            
        return None
    
    def resultType(self):
        return self._result

    def __eq__(self, other):
        (done, result) = self._eq_common(other)
        if done:
            return result
        
        
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
        (done, result) = self._eq_common(other)
        if done:
            return result
        
        return True
        
class TupleType(Type):
    def __init__(self, types, generic=False):
        docname = generic and "(tuple)" or None
        super(TupleType, self).__init__("(%s)" % ", ".join([t.name() for t in types]), docname)
        self._types = types
        self._generic = generic

    def __eq__(self, other):
        (done, result) = self._eq_common(other)
        if done:
            return result
        
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
        #(done, result) = self._eq_common(other)
        #if done:
        #    return result
        
        if isinstance(other, OneOf):
            return self._types == other._types

        return other in self._types

###

Int16 = IntegerType(16)        
Int32 = IntegerType(32)        
Integer = IntegerType()

String = StringType("string")
Bool = BoolType("bool")
Label = LabelType("label")
Void = VoidType("void")
Any = AnyType("any")
AnyTuple = TupleType((), generic=True)
Function = FunctionType(generic=True)

def Optional(t):
    return OneOf((t, None))

# Returns a readable respresentation of the given type (which might be a tuple of types)
def name(t, docstring = False):
    # FIXME: Get rid fo this.
    
    if not docstring:
        return t.name()
    else:
        return t.docname()

_types = {
	"int": Integer, 
	"int16": Int16, # Short-cut
	"int32": Int32, # Short-cut
    "string": String,
    "bool": Bool,
    "void": Void,
    }

# Turns a type name as specificed in the input code into an actual type. 
def type(name, width=0):
    
    if width:
        # FIXME: Not nice to hard-code that here. 
        if name == "int":
            return IntegerType(width)
        else:
            return None
    
    try:
        return _types[name]
    except KeyError:
        return None

# Returns a list of all type names.
def typenames():
    return _types.keys()

    
# Returns true if type is options.
def isOptional(type):
    return isinstance(type, OneOf) and type == None
    
