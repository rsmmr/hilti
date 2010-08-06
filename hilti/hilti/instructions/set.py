# $Id$
"""
Sets
~~~~

A ``set`` store keys of a particular type ``K``. Insertions,
lookups, and deletes are amortized constant time. Keys must be HILTI
*value types*, and each value can only be stored once.  

Sets are iterable, yet the order in which elements are iterated over
is undefined. 

Todo: Add note on semantics when modifying the hash table while iterating over
it.

Todo: When resizig, load spikes can occur for large set. We should
extend the hash table implementation to do resizes incrementally. 
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type(None, 101)
class IteratorSet(type.Iterator):
    """Type for iterating over a ``set``.
    
    t: ~~Set - The set type.
    """
    def __init__(self, t, location=None):
        super(IteratorSet, self).__init__(t, location)
        
    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_set_iter"
        return typeinfo
    
    def llvmType(self, cg):
        """An ``iterator<set<K>>`` is mapped to ``struct hlt_set_iter``."""
        # Must match with what the C implementation uses as iterator type.
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer(), llvm.core.Type.int(64)])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """Set iterators are initially equivalent to ``set.end``."""
        # Must match with what the C implementation expects as end() marker.
        return llvm.core.Constant.struct([llvm.core.Constant.null(cg.llvmTypeGenericPointer()), cg.llvmConstInt(0, 64)])

    ### Overridden from Iterator.
    
    def derefType(self):
        t = self.parentType().itemType() 
        return t if t else type.Any()
    
@hlt.type("set", 15)
class Set(type.Container, type.Iterable):
    """Type for a ``set``.
    
    key: ~~Type - The key's type.
    value: ~~Type - The value's type.
    
    Note: The container's item type is ``tuple<key,value>``. 
    """
    def __init__(self, key, location=None):
        super(Set, self).__init__(key, location)
        self._key = key

    def keyType(self):
        """Returns the type of the set's keys.
        
        Returns: ~~Type - The key type.
        """
        return self._key if self._key else type.Any()
    
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``set`` is mapped to a ``hlt_set *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_set *"
        typeinfo.to_string = "hlt::set_to_string"
        return typeinfo

    ### Overriden from Iterable.
        
    def iterType(self):
        return IteratorSet(self, location=self.location())

def _cKeyTypeOfOp1():
    """Returns a constraint function that ensures that the operand is of a
    set's ~~keyType."""
    @hlt.constraint("*key-type*")
    def _keyTypeOf(ty, op, i):
        it = i.op1().type().refType().keyType()
        return (op.canCoerceTo(it), "must be of type %s but is %s" % (it, ty))
    
    return _keyTypeOf

@hlt.overload(New, op1=cType(cSet), op2=cOptional(cReferenceOf(cTimerMgr)), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a set. If automatic item expiration is to
    be used, a timer manager must be given as *op2*.
    """
    def _codegen(self, cg):
        key = operand.Type(self.op1().value().keyType())
        
        if self.op2():
            tmgr = self.op2()
        else: 
            tmgr = operand.LLVM(llvm.core.Constant.null(cg.llvmTypeGenericPointer()), type.Reference(type.TimerMgr()))
            
        result = cg.llvmCallC("hlt::set_new", [key, tmgr])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("set.insert", op1=cReferenceOf(cSet), op2=_cKeyTypeOfOp1())
class Insert(Instruction):
    """Sets the element at index *op2* in set *op1* to *op3. If the key
    already exists, the previous entry is replaced."""
    def _codegen(self, cg):
        key = self.op1().type().refType().keyType()
        cg.llvmCallC("hlt::set_insert", [self.op1(), self.op2().coerceTo(cg, key)])
        
@hlt.instruction("set.timeout", op1=cReferenceOf(cSet), op2=cEnum, op3=cDouble)
class Timeout(Instruction):
    """Activates automatic expiration of items for set *op1*. All subsequently
    inserted entries will be expired *op3* seconds after they have been added
    (if *op2* is *Expire::Create*) or last accessed (if *op2* is
    *Expire::Access). Expiration is disable if *op3* is zero. Throws
    NoTimerManager if no timer manager has been associated with the set at
    construction.""" 
    def _codegen(self, cg):
        cg.llvmCallC("hlt::set_timeout", [self.op1(), self.op2(), self.op3()])
        
@hlt.instruction("set.exists", op1=cReferenceOf(cSet), op2=_cKeyTypeOfOp1(), target=cBool)
class Exists(Instruction):
    """Checks whether the key *op2* exists in set *op1*. If so, the
    instruction returns True, and False otherwise."""
    def _codegen(self, cg):
        key = self.op1().type().refType().keyType()
        result = cg.llvmCallC("hlt::set_exists", [self.op1(), self.op2().coerceTo(cg, key)])
        cg.llvmStoreInTarget(self, result)
        
@hlt.instruction("set.remove", op1=cReferenceOf(cSet), op2=_cKeyTypeOfOp1())
class Remove(Instruction):
    """Removes the key *op2* from the set *op1*. If the key does not exists,
    the instruction has no effect."""
    def _codegen(self, cg):
        key = self.op1().type().refType().keyType()
        cg.llvmCallC("hlt::set_remove", [self.op1(), self.op2().coerceTo(cg, key)])
    
@hlt.instruction("set.size", op1=cReferenceOf(cSet), target=cIntegerOfWidth(64))
class Size(Instruction):
    """Returns the current number of entries in set *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::set_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)
        
@hlt.instruction("set.clear", op1=cReferenceOf(cSet))
class Clear(Instruction):
    """Removes all entries from set *op1*."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::set_clear", [self.op1()])

@hlt.overload(Begin, op1=cReferenceOf(cSet), target=cIteratorSet)
class Begin(Operator):
    """Returns an iterator representing the first element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::set_begin", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cSet), target=cIteratorSet)
class End(Operator):
    """Returns an iterator representing the position one after the last
    element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::set_end")
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorSet, target=cIteratorSet)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::set_iter_incr", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorSet, target=cDerefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at as a tuple ``(key,value)``.
    
    Note: Dereferencing an iterator does not count as an access to the element
    for restarting its expiration timer. 
    """
    def _codegen(self, cg):
        t = self.target().type()
        voidp = cg.llvmCallC("hlt::set_iter_deref", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.overload(Equal, op1=cIteratorSet, op2=cIteratorSet, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::set_iter_eq", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)
