# $Id$
#
# The list type.

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

@type.pac("list")
class List(type.ParseableContainer):
    """Type for list objects.

    ty: ~~Type or ~~Expr - The type of the container elements. If an
    expression, its type defines the item type.

    value: ~~Expr - If the container elements are specified via a constant,
    this expression gives that (e.g., regular expression constants for parsing
    bytes). None otherwise.

    itemargs: list of ~~Expr - Optional parameters for parsing the items.
    Only valid if the item type is a ~~Unit.  None otherwise.

    location: ~~Location - A location object describing the point of
    definition.
    """
    def __init__(self, ty, value=None, item_args=None, location=None):
        super(List, self).__init__(ty, value, item_args, location=location)

    ### Overridden from Type.

    def hiltiCtor(self, cg, value):
        elems = [e.evaluate(cg) for e in value]
        ltype = hilti.type.Reference(hilti.type.List(self.itemType().hiltiType(cg)))
        return hilti.operand.Ctor(elems, ltype)

    def hiltiType(self, cg):
        ltype = hilti.type.List(self.itemType().hiltiType(cg))
        return hilti.type.Reference(ltype)

    def hiltiDefault(self, cg, must_have=True):
        ltype = hilti.type.List(self.itemType().hiltiType(cg))
        return hilti.operand.Ctor([], hilti.type.Reference(ltype))

    def hiltiUnitDefault(self, cg):
        return self.hiltiDefault(cg)

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {
            "length": (type.UnsignedInteger(64), False, None),
            "until": (type.Bool(), False, None)
            }

    ### Overridden from ParseableContainer.

    def containerProduction(self, field, item):
        loc = field.location()
        unit = field.parent()

        until = self.attributeExpr("until")
        length = self.attributeExpr("length")

        if until:
            # Create a even higer priority hook that checks whether the &until
            # condition has been reached.
            hook = stmt.FieldForEachHook(unit, field, 255)

            stop = stmt.Return(expr.Ctor(False, type.Bool()), _hook=True)
            ifelse = stmt.IfElse(until, stop, None)

            hook.setStmts(stmt.Block(None, stmts=[ifelse]))
            unit.module().addHook(hook)

            # Add a boolean production checking the condition.
            #
            # List1 -> Item Alt
            # List2 -> Epsilon
            hookrc = expr.Hilti(hilti.operand.ID(hilti.id.Local("__hookrc", hilti.type.Bool())), type.Bool())
            l1 = grammar.Sequence(location=loc)
            eps = grammar.Epsilon(location=loc)
            alt = grammar.Boolean(hookrc, l1, eps, location=loc)
            l1.addProduction(item)
            l1.addProduction(alt)

            item.setUntilField(field)

        elif length:
            l1 = grammar.Counter(length, item, location=loc)

        else:
            # No attributes, use look-ahead to figure out when to stop parsing.

            # Left-factored & right-recursive.
            #
            # List1 -> Item List2
            # List2 -> Epsilon | List1
            l1 = grammar.Sequence(location=loc)
            l2 = grammar.LookAhead(grammar.Epsilon(), l1, location=loc)
            l1.addProduction(item)
            l1.addProduction(l2)

            l1 = l2

        return l1


def itemType(exprs):
    return exprs[0].type().itemType()

@operator.MethodCall(operator.Mutable(List), expr.Attribute("push_back"), [itemType])
class PushBack:
    def type(obj, method, args):
        return obj.type()

    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        item = args[0].evaluate(cg)
        cg.builder().list_push_back(obj, item)
        return obj

@operator.PlusAssign(List, List)
class Plus:
    def type(l1, l2):
        return type.Bytes()

    def validate(vld, l1, l2):
        if l1.type().itemType() != l2.type().itemType():
            vld.error(l1, "incompatible lists")

    def evaluate(cg, l1, l2):
        l1 = l1.evaluate(cg)
        l2 = l2.evaluate(cg)

        ty = l2.type().refType()

        i = cg.functionBuilder().addLocal("__i", ty.iterType())
        c = cg.functionBuilder().addLocal("__cond", hilti.type.Bool())
        item = cg.functionBuilder().addLocal("__item", ty.itemType())
        end = cg.functionBuilder().addLocal("__end", ty.iterType())

        def init(builder, next):
            builder.begin(i, l2)
            builder.end(end, l2)
            builder.jump(next)

        def cond(builder, next):
            builder.unequal(c, i, end)
            builder.jump(next)
            return c

        def body(builder, next):
            builder.deref(item, i)
            builder.list_push_back(l1, item)
            builder.incr(i, i)
            builder.jump(next)

        next = cg.builder().makeForLoop(init, cond, body, tag="list_append")
        cg.setBuilder(next)

        return l1
