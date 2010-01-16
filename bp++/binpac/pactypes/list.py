# $Id$
#
# The list type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.stmt as stmt
import binpac.core.id as id
import binpac.core.constant as constant
import binpac.core.printer as printer

import binpac.core.operator as operator
import binpac.core.grammar as grammar
import binpac.support.util as util

import hilti.core.type

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

    def validate(self, vld):
        return True
    
    def validateConst(self, vld, const):
        if not isinstance(const.value(), list):
            vld.error(const, "list: constant of wrong internal type")
            
        for elem in const.value():
            if elem.type() != self._item:
                vld.error(const, "list: constant must be of type %s" % elem.type())
            
            elem.validate(vld)

    def hiltiConstant(self, cg, const):
        elems = [hilti.core.instruction.ConstOperand(self._item.hiltiConstant(cg, c)) for c in const.value()]
        return hilti.core.constant.Constant(elems, const.type().hiltiType(cg))
            
    def hiltiType(self, cg):
        return hilti.core.type.Reference([hilti.core.type.List([self._item.hiltiType(cg)])])

    def hiltiDefault(self, cg):
        return hilti.core.constant.Constant([], self.hiltiType(cg))

    def resolve(self, resolver):
        self._item = self._item.resolve(resolver)
        return self
    
    def pac(self, printer):
        printer.output("list<")
        self._item.pac(printer)
        printer.output(">")

    def pacConstant(self, printer, const):
        printer.output("[")
        elems = const.value()
        for i in range(len(elems)):
            self._item.pacConstant(printer, elems[i])
            if i != len(elems) - 1:
                printer.output(", ")
        printer.output("]")

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self, field):
        loc = self.location()
        # Left-factored & right-recursive. 
        #
        # List1 -> Item List2
        # List2 -> Epsilon | List1
        l1 = grammar.Sequence(location=loc)
        l2 = grammar.LookAhead(grammar.Epsilon(), l1, location=loc)
        item = self.itemType().production(field)
        l1.addProduction(item)
        l1.addProduction(l2) 
        
        loc = field.location()

        # Create a hook "list.push_back($$)".
        
        hook = stmt.FieldHook(field, 255, location=loc)
        
        dollar = expr.Name("__dollardollar", hook.scope(), location=loc)
        self = expr.Name("self", hook.scope(), location=loc)
        list = expr.Constant(constant.Constant(field.name(), type.Identifier()), location=loc)
        push_back = "push_back" #expr.Constant(constant.Constant("push_back", type.Identifier()), location=loc)
        
        attr = expr.Overloaded(operator.Operator.Attribute, (self, list), location=loc)
        call = expr.Overloaded(operator.Operator.MethodCall, (attr, push_back, [dollar]), location=loc)

        hook.addStatement(stmt.Expression(call))
        
        item.addHook(hook)
        
        return l1

    def dollarDollarType(self, field):
        return field.type().itemType()
    
def itemType(exprs):
    return exprs[0].type().itemType()
        
@operator.MethodCall(operator.Mutable(List), "push_back", [itemType])
class PushBack:
    def type(obj, method, args):
        return obj.type()
    
    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        item = args[0].evaluate(cg)
        cg.builder().list_push_back(obj, item)
        return obj
        
