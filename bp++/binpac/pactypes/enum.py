# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.id as id
import binpac.operator as operator
import binpac.constant as constant

import hilti.type

@type.pac("enum")
class Enum(type.Type):
    """Type for enumeration values. In addition to the specificed labels, an
    additional ``Undef`` value is implicitly defined and is the default value
    for all not otherwise initialized values. 
    
    labels: list of strings - The labels. 
    
    scope: ~~Scope - Scope into which the labels are inserted.
    
    namesapce: string  - The namespace for the labels (i.e., the name of the
    enum type).
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, labels, scope, namespace, location=None):
        super(Enum, self).__init__(location=location)
        self._scope = scope
        self._namespace = namespace
        self._labels = labels
        self._labels.sort()
        
        self._hlttype = None
        
        for label in self._labels + ["Undef"]:
            eid = id.Constant(namespace + "::" + label, self, expr.Ctor(label, self))
            scope.addID(eid)
            
    ### Overridden from Type.
    
    def validate(self, vld):
        if "Undef" in self._labels:
            vld.error(self, "Undef enum value already predefined.")

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, str):
            vld.error(ctor, "enum: ctor of wrong internal type")
            
        if not ctor in self._labels + ["Undef"]:
            vld.error("undefined enumeration label '%s'" % ctor)

    def supportedAttributes(self):
        return { "default": (self, True, None) }            
            
    def hiltiType(self, cg):
        if not self._hlttype:
            self._hlttype = cg.moduleBuilder().addEnumType(self._namespace, self._labels)
        
        return self._hlttype
    
    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, self.hiltiType(cg))
        return hilti.operand.Constant(const)
    
    def name(self):
        return "enum" 
    
    def pac(self, printer):
        printer.output("enum { %s }" % ", ".join(self._labels))
        
    def pacCtor(self, printer, value):
        printer.output(value)
    
@operator.Equal(type.Enum, type.Enum)
class _:
    def type(e1, e2):
        return type.Bool()
    
    def validate(vld, e1, e2):
        if e1.type()._labels != e2.type()._labels:
            vld.error("incompatible enum types")
    
    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None
            
        b = (e1.value() == e2.value())
        return expr.Ctor(b, type.Bool())
        
    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp
        
