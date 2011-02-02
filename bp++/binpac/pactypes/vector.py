# $Id$
#
# The vector type.

builtin_id = id

import binpac.type as type
import binpac.expr as expr
import binpac.stmt as stmt
import binpac.id as id
import binpac.printer as printer

import binpac.operator as operator
import binpac.grammar as grammar
import binpac.util as util

import unit
import function

import hilti.type
import hilti.operand

import copy

@type.pac("vector")
class Vector(type.ParseableContainer):
    """Type for vector objects.

    ty: ~~Type or ~~Expr - The type of the container elements. If an
    expression, its type defines the item type.

    value: ~~Expr - If the container elements are specified via a constant,
    this expression gives that (e.g., regular expression constants for parsing
    bytes). None otherwise.

    itemargs: vector of ~~Expr - Optional parameters for parsing the items.
    Only valid if the item type is a ~~Unit.  None otherwise.

    length: ~~Expr - A length expression for parsing the type inside a unit.

    location: ~~Location - A location object describing the point of
    definition.
    """
    def __init__(self, ty, value=None, item_args=None, length=None, location=None):
        super(Vector, self).__init__(ty, value, item_args, location=location)
        self._length = length

    ### Overridden from Type.

    def hiltiCtor(self, cg, value):
        elems = [e.evaluate(cg) for e in value]
        ltype = hilti.type.Reference(hilti.type.Vector(self.itemType().hiltiType(cg)))
        return hilti.operand.Ctor(elems, ltype)

    def hiltiType(self, cg):
        ltype = hilti.type.Vector(self.itemType().hiltiType(cg))
        return hilti.type.Reference(ltype)

    def hiltiDefault(self, cg, must_have=True):
        ltype = hilti.type.Vector(self.itemType().hiltiType(cg))
        return hilti.operand.Ctor([], hilti.type.Reference(ltype))

    def hiltiUnitDefault(self, cg):
        return self.hiltiDefault(cg)

    ### Overridden from ParseableType.

    def validateInUnit(self, field, vld):
        if not self._length:
            util.internal_error(vld, "vector inside unit does not have length expression")

    ### Overridden from ParseableContainer.

    def containerProduction(self, field, item):
        assert self._length
        return grammar.Counter(self._length, item, location=field.location())

def _itemType(exprs):
    return exprs[0].type().itemType()

@operator.Index(Vector, type.UnsignedInteger)
class _:
    def type(v, i):
        return v.type().itemType()

    def evaluate(cg, v, i):
        it = v.type().itemType()
        tmp = cg.functionBuilder().addLocal("__elem", it.hiltiType(cg))
        cg.builder().vector_get(tmp, v.evaluate(cg), i.evaluate(cg))
        return tmp

@operator.IndexAssign(Vector, type.UnsignedInteger, _itemType)
class _:
    def type(v, i, val):
        return t.type().itemType()

    def evaluate(cg, v, i, val):
        it = v.type().itemType()
        val = val.evaluate(cg)
        cg.builder().vector_set(tmp, v.evaluate(cg), i.evaluate(cg), val)
        return val

@operator.Size(Vector)
class _:
    def type(e):
        return type.UnsignedInteger(64)

    def evaluate(cg, e):
        tmp = cg.functionBuilder().addLocal("__size", hilti.type.Integer(64))
        cg.builder().vector_size(tmp, e.evaluate(cg))
        return tmp

@operator.MethodCall(operator.Mutable(Vector), expr.Attribute("push_back"), [_itemType])
class PushBack:
    def type(obj, method, args):
        return obj.type()

    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        item = args[0].evaluate(cg)
        cg.builder().vector_push_back(obj, item)
        return obj

@operator.MethodCall(operator.Mutable(Vector), expr.Attribute("reserve"), [type.UnsignedInteger])
class PushBack:
    def type(obj, method, args):
        return obj.type()

    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        size = args[0].evaluate(cg)
        cg.builder().vector_reserve(obj, size)
        return obj
