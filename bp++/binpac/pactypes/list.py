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

import hilti.type
import hilti.operand

import copy

@type.pac("list")
class List(type.ParseableType):
    """Type for list objects.  
    
    itemty: ~~Type - The type of the list elements. 
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, itemty, location=None):
        super(List, self).__init__(location=location)
        self._item = itemty

    def itemType(self):
        """Returns the type of the list elements.
        
        Returns: ~~Type - The type of the list elements.
        """
        return self._item
        
    ### Overridden from Type.
    
    def name(self):
        return "list<%s>" % self._item.name()

    def doResolve(self, resolver):
        self._item = self._item.resolve(resolver)
    
    def validate(self, vld):
        type.ParseableType.validate(self, vld)
        self._item.validate(vld)
    
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
        ltype = hilti.type.List(self._item.hiltiType(cg))
        return hilti.operand.Ctor(elems, ltype)
            
    def hiltiType(self, cg):
        ltype = hilti.type.List(self._item.hiltiType(cg))
        return hilti.type.Reference(ltype)

    def hiltiDefault(self, cg, must_have=True):
        ltype = hilti.type.List(self._item.hiltiType(cg))
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
        ctlhook = stmt.FieldControlHook(field, 255, copy.deepcopy(self.itemType()))
        field.setControlHook(ctlhook)
    
    def production(self, field):
        loc = self.location()
        until = self.attributeExpr("until")
        hook = field.controlHook()
        assert hook
        
        # Create a "list.push_back($$)" statement. 
        dollar = expr.Name("__dollardollar", hook.scope(), location=loc)
        slf = expr.Name("self", hook.scope(), location=loc)
        list = expr.Attribute(field.name(), location=loc)
        method = expr.Attribute("push_back", location=loc)
        attr = expr.Overloaded(operator.Operator.Attribute, (slf, list), location=loc)
        push_back = expr.Overloaded(operator.Operator.MethodCall, (attr, method, [dollar]), location=loc)
        push_back = stmt.Expression(push_back, location=loc)
        
        if until:
            # &until(expr)
             
            # List1 -> Item Alt
            # List2 -> Epsilon
            hookrc = expr.Name("__hookrc", hook.scope())
            l1 = grammar.Sequence(location=loc)
            eps = grammar.Epsilon(location=loc)
            alt = grammar.Boolean(hookrc, l1, eps, location=loc)
            item = self.itemType().production(field)
            l1.addProduction(item)
            l1.addProduction(alt)
            
            # if ( <until-expr> ) 
            #     __hookrc = False
            # else
            #     list.push_back($$)
            
            stop = stmt.Expression(expr.Assign(hookrc, expr.Ctor(False, type.Bool())))
            ifelse = stmt.IfElse(until, stop, push_back, location=loc)
            
            hook.addStatement(ifelse)
            item.addHook(hook)
            
        else:
            # No attributes, use look-ahead to figure out when to stop parsing. 
            
            # Left-factored & right-recursive. 
            #
            # List1 -> Item List2
            # List2 -> Epsilon | List1
            l1 = grammar.Sequence(location=loc)
            l2 = grammar.LookAhead(grammar.Epsilon(), l1, location=loc)
            item = self.itemType().production(field)
            l1.addProduction(item)
            l1.addProduction(l2) 
            
            #     list.push_back($$)
            hook.addStatement(push_back)        
            item.addHook(hook)
            
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
        
