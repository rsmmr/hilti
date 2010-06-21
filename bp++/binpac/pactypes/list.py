# $Id$
#
# The list type.

import binpac.type as type
import binpac.expr as expr
import binpac.stmt as stmt
import binpac.id as id
import binpac.constant as constant
import binpac.printer as printer

import binpac.operator as operator
import binpac.grammar as grammar
import binpac.util as util

import unit

import hilti.type
import hilti.operand

import copy

@type.pac("list")
class List(type.ParseableType):
    """Type for list objects.  
    
    itemty: ~~Type - The type of the list elements. 
    value: ~~Expr - The value for lists of constants, or None otherwise. 
    itemargs: list of ~~Expr - Optional parameters for parsing the items. Only valid
    if the type is a ~~Unit and used inside another unit's field.
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, itemty, value=None, item_args=None, location=None):
        super(List, self).__init__(location=location)
        self._item = itemty
        self._item_args = item_args
        self._prod = None
        self._value = value
        self._field = None

    def itemType(self):
        """Returns the type of the list elements.
        
        Returns: ~~Type - The type of the list elements.
        """
        return self._item.parsedType()
        
    ### Overridden from Type.
    
    def name(self):
        return "list<%s>" % self._item.name()

    def doResolve(self, resolver):
        if self._item:
            # FIXME: This block is copied from unit.Field.resolve
            old_item = self._item

            # Before we resolve the type, let's see if it's actually refering
            # to a constanst.
            if isinstance(self._item, type.Unknown):
                i = resolver.scope().lookupID(self.itemType().idName())
                
                if i and isinstance(i, id.Constant):
                    self._value = i.expr()
                    self._item = i.expr().type()
                else:
                    self._item = self._item.resolve(resolver)
                    
            else:
                self._item = self._item.resolve(resolver)
                
            self._item.copyAttributesFrom(old_item)
            
        if self._value:
            self._value.resolve(resolver)
            
        if self._item_args:
            for p in self._item_args:
                p.resolve(resolver)
                
        if self._field:
            for ch in self._field.controlHooks():
                ch.setDollarDollarType(self.itemType())
        
    def validate(self, vld):
        type.ParseableType.validate(self, vld)
        self._item.validate(vld)
        
        if self._value:
            self._value.validate(vld)
        
        if self._item_args and not isinstance(self._item, type.Unit):
            vld.error(self, "parameters only allowed for unit types")
            
        if self._item_args:
            for p in self._item_args:
                p.validate(vld)
    
    def validateCtor(self, vld, value):
        if not isinstance(value, list):
            vld.error(self, "list: ctor value of wrong internal type")
            
        for elem in value:
            if not isinstance(elem, expr.Expression):
                vld.error(self, "list: ctor value's elements of wrong internal type")
            
        for elem in value:
            if elem.type() != self._item:
                vld.error(self, "list: ctor value must be of type %s" % elem.type())
            
            elem.validate(vld)

    def hiltiCtor(self, cg, value):
        elems = [e.evaluate(cg) for e in value]
        ltype = hilti.type.List(self.itemType().hiltiType(cg))
        return hilti.operand.Ctor(elems, ltype)
            
    def hiltiType(self, cg):
        ltype = hilti.type.List(self.itemType().hiltiType(cg))
        return hilti.type.Reference(ltype)

    def hiltiDefault(self, cg, must_have=True):
        ltype = hilti.type.List(self.itemType().hiltiType(cg))
        return hilti.operand.Ctor([], hilti.type.Reference(ltype))

    def pac(self, printer):
        printer.output("list<")
        self._item.pac(printer)
        printer.output(">")

    def pacCtor(self, printer, elems):
        printer.output("[")
        for i in range(len(elems)):
            self._item.pacCtor(printer, elems[i])
            if i != len(elems) - 1:
                printer.output(", ")
        printer.output("]")

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { "until": (type.Bool(), False, None) }

    def initParser(self, field):
        ctlhook = stmt.InternalControlHook(field, 255, None)
        field.addControlHook(ctlhook)
        
        self._field = field
        
    def _itemProduction(self, field):
        """Todo: This is pretty ugly and mostly copied from
        ~~unit.Field.production. We should probably use a ~~unit.Field as our
        item type directly."""
        if self._value:
            for t in unit._AllowedConstantTypes:
                if isinstance(self._item, t):
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
        
    def production(self, field):
        if self._prod:
            return self._prod
        
        loc = self.location()
        until = self.attributeExpr("until")

        internal_hook = None
        foreach_hooks = []
        
        for hook in field.controlHooks():
            if isinstance(hook, stmt.InternalControlHook):
                internal_hook = hook
                
            if isinstance(hook, stmt.ForEachHook):
                foreach_hooks += [hook]
                
        assert internal_hook
        
        if field.name():
            # Create a "list.push_back($$)" statement for the internal_hook.
            dollar = expr.Name("__dollardollar", internal_hook.scope(), location=loc)
            slf = expr.Name("self", internal_hook.scope(), location=loc)
            list = expr.Attribute(field.name(), location=loc)
            method = expr.Attribute("push_back", location=loc)
            attr = expr.Overloaded(operator.Operator.Attribute, (slf, list), location=loc)
            push_back = expr.Overloaded(operator.Operator.MethodCall, (attr, method, [dollar]), location=loc)
            push_back = stmt.Expression(push_back, location=loc)
        else:
            push_back = None
        
        item = self._itemProduction(field)
        
        if until:
            # &until(expr)
             
            # List1 -> Item Alt
            # List2 -> Epsilon
            hookrc = expr.Name("__hookrc", internal_hook.scope())
            l1 = grammar.Sequence(location=loc)
            eps = grammar.Epsilon(location=loc)
            alt = grammar.Boolean(hookrc, l1, eps, location=loc)
            l1.addProduction(item)
            l1.addProduction(alt)
            
            # if ( <until-expr> ) 
            #     __hookrc = False
            # else
            #     list.push_back($$)
            
            stop = stmt.Expression(expr.Assign(hookrc, expr.Ctor(False, type.Bool())))
            ifelse = stmt.IfElse(until, stop, push_back, location=loc)
            
            internal_hook.addStatement(ifelse)
            
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
            
            # list.push_back($$)
            if push_back:
                internal_hook.addStatement(push_back)        
            
        item.addHook(internal_hook)
        
        for hook in foreach_hooks:
            item.addHook(hook)
        
        self._prod = l1
        return self._prod
            
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
        
