# $Id$
"""
Maps
~~~~

A ``maps`` maps keys of type ``K`` to values of type ``T``. Insertions,
lookups, and deletes are amortized constant time. Keys must be HILTI *value
types*, while values can be of any time. Up to one value can be associated
with each key. 

Maps are iterable, yet the order in which elements are iterated over
is undefined. 

Todo: Add note on semantics when modifying the hash table while iterating over
it.

Todo: When resizig, load spikes can occur for large tables. We should extend
the hash table implementation to do resizes incrementally. 
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type(None, 101)
class IteratorMap(type.Iterator):
    """Type for iterating over a ``map``.
    
    t: ~~Map - The map type.
    """
    def __init__(self, t, location=None):
        super(IteratorMap, self).__init__(t, location)
        
    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_map_iter"
        return typeinfo
    
    def llvmType(self, cg):
        """An ``iterator<map<K,V>>`` is mapped to ``struct hlt_map_iter``."""
        # Must match with what the C implementation uses as iterator type.
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer(), llvm.core.Type.int(64)])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """Map iterators are initially equivalent to ``map.end``."""
        # Must match with what the C implementation expects as end() marker.
        return llvm.core.Constant.struct([llvm.core.Constant.null(cg.llvmTypeGenericPointer()), cg.llvmConstInt(0, 64)])

    ### Overridden from Iterator.
    
    def derefType(self):
        return self.parentType().itemType() 
    
@hlt.type("map", 15)
class Map(type.Container, type.Iterable):
    """Type for a ``map``.
    
    key: ~~Type - The key's type.
    value: ~~Type - The value's type.
    
    Note: The container's item type is ``tuple<key,value>``. 
    """
    def __init__(self, key, value, location=None):
        t = type.Tuple([key if key else type.Any(), value if value else type.Any()])
        super(Map, self).__init__(t, location)
        self._key = key
        self._value = value

    def keyType(self):
        """Returns the type of the map's keys.
        
        Returns: ~~Type - The key type.
        """
        return self._key if self._key else type.Any()
    
    def valueType(self):
        """Returns the type of the map's values.
        
        Returns: ~~Type - The value type.
        """
        return self._value if self._value else type.Any()
        
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``map`` is mapped to a ``hlt_map *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_map *"
        typeinfo.to_string = "hlt::map_to_string"
        return typeinfo

    ### Overriden from Iterable.
        
    def iterType(self):
        return IteratorMap(self, location=self.location())

def _cKeyTypeOfOp1():
    """Returns a constraint function that ensures that the operand is of a
    map's ~~keyType."""
    @hlt.constraint("*key-type*")
    def _keyTypeOf(ty, op, i):
        it = i.op1().type().refType().keyType()
        return (op.canCoerceTo(it), "must be of type %s but is %s" % (it, ty))
    
    return _keyTypeOf

def _cValueTypeOfOp1():
    """Returns a constraint function that ensures that the operand is of a
    map's ~~valueType."""
    @hlt.constraint("*value-type*")
    def _valueTypeOf(ty, op, i):
        it = i.op1().type().refType().valueType()
        return (op.canCoerceTo(it), "must be of type %s but is %s" % (it, ty))
    
    return _valueTypeOf
    
@hlt.overload(New, op1=cType(cMap), op2=cOptional(cReferenceOf(cTimerMgr)), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a map. If automatic item expiration is to
    be used, a timer manager must be given as *op2*.
    """
    def codegen(self, cg):
        key = operand.Type(self.op1().value().keyType())
        value = operand.Type(self.op1().value().valueType())
        
        if self.op2():
            tmgr = self.op2()
        else: 
            tmgr = operand.LLVM(llvm.core.Constant.null(cg.llvmTypeGenericPointer()), type.Reference(type.TimerMgr()))
            
        result = cg.llvmCallC("hlt::map_new", [key, value, tmgr])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("map.get", op1=cReferenceOf(cMap), op2=_cKeyTypeOfOp1(), target=_cValueTypeOfOp1())
class Get(Instruction):
    """Returns the element with key *op2* in map *op1*. Throws IndexError if
    the key does not exists and no default has been set via *map.default*."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        value = self.op1().type().refType().valueType()
        voidp = cg.llvmCallC("hlt::map_get", [self.op1(), self.op2().coerceTo(cg, key)])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(value)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))
        
@hlt.instruction("map.get_default", op1=cReferenceOf(cMap), op2=_cKeyTypeOfOp1(), op3=_cValueTypeOfOp1(), target=_cValueTypeOfOp1())
class GetDefault(Instruction):
    """Returns the element with key *op2* in map *op1* if it exists. If the
    key does not exists, returns *op3*."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        value = self.op1().type().refType().valueType()
        voidp = cg.llvmCallC("hlt::map_get_default", [self.op1(), self.op2().coerceTo(cg, key), self.op3().coerceTo(cg, value)])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(value)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.instruction("map.insert", op1=cReferenceOf(cMap), op2=_cKeyTypeOfOp1(), op3=_cValueTypeOfOp1())
class Insert(Instruction):
    """Sets the element at index *op2* in map *op1* to *op3. If the key
    already exists, the previous entry is replaced."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        value = self.op1().type().refType().valueType()
        cg.llvmCallC("hlt::map_insert", [self.op1(), self.op2().coerceTo(cg, key), self.op3().coerceTo(cg, value)])
        
@hlt.instruction("map.default", op1=cReferenceOf(cMap), op2=_cValueTypeOfOp1())
class Default(Instruction):
    """Sets a default value *op2* for map *op1* to be returned by *map.get* if
    a key does not exist."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        value = self.op1().type().refType().valueType()
        cg.llvmCallC("hlt::map_default", [self.op1(), self.op2().coerceTo(cg, value)])
        
@hlt.instruction("map.timeout", op1=cReferenceOf(cMap), op2=cEnum, op3=cDouble)
class Timeout(Instruction):
    """Activates automatic expiration of items for map *op1*. All subsequently
    inserted entries will be expired *op3* seconds after they have been added
    (if *op2* is *Expire::Create*) or last accessed (if *op2* is
    *Expire::Access). Expiration is disable if *op3* is zero. Throws
    NoTimerManager if no timer manager has been associated with the map at
    construction.""" 
    def codegen(self, cg):
        cg.llvmCallC("hlt::map_timeout", [self.op1(), self.op2(), self.op3()])
        
@hlt.instruction("map.exists", op1=cReferenceOf(cMap), op2=_cKeyTypeOfOp1(), target=cBool)
class Exists(Instruction):
    """Checks whether the key *op2* exists in map *op1*. If so, the
    instruction returns True, and False otherwise."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        result = cg.llvmCallC("hlt::map_exists", [self.op1(), self.op2().coerceTo(cg, key)])
        cg.llvmStoreInTarget(self, result)
        
@hlt.instruction("map.remove", op1=cReferenceOf(cMap), op2=_cKeyTypeOfOp1())
class Remove(Instruction):
    """Removes the key *op2* from the map *op1*. If the key does not exists,
    the instruction has no effect."""
    def codegen(self, cg):
        key = self.op1().type().refType().keyType()
        cg.llvmCallC("hlt::map_remove", [self.op1(), self.op2().coerceTo(cg, key)])
    
@hlt.instruction("map.size", op1=cReferenceOf(cMap), target=cIntegerOfWidth(64))
class Size(Instruction):
    """Returns the current number of entries in map *op1*."""
    def codegen(self, cg):
        result = cg.llvmCallC("hlt::map_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)
        
@hlt.instruction("map.clear", op1=cReferenceOf(cMap))
class Clear(Instruction):
    """Removes all entries from map *op1*."""
    def codegen(self, cg):
        cg.llvmCallC("hlt::map_clear", [self.op1()])

@hlt.instruction("map.begin", op1=cReferenceOf(cMap), target=cIteratorMap)
class Begin(Instruction):
    """Returns an iterator representing the first element of *op1*."""
    def codegen(self, cg):
        result = cg.llvmCallC("hlt::map_begin", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("map.end", op1=cReferenceOf(cMap), target=cIteratorMap)
class End(Instruction):
    """Returns an iterator representing the position one after the last
    element of *op1*."""
    def codegen(self, cg):
        result = cg.llvmCallC("hlt::map_end")
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorMap, target=cIteratorMap)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    def codegen(self, cg):
        result = cg.llvmCallC("hlt::map_iter_incr", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorMap, target=cDerefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at as a tuple ``(key,value)``.
    
    Note: Dereferencing an iterator does not count as an access to the element
    for restarting its expiration timer. 
    """
    def codegen(self, cg):
        t = self.target().type()
        returnt = operand.Type(self.op1().type().derefType())
        voidp = cg.llvmCallC("hlt::map_iter_deref", [returnt, self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.overload(Equal, op1=cIteratorMap, op2=cIteratorMap, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    def codegen(self, cg):
        result = cg.llvmCallC("hlt::map_iter_eq", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)
