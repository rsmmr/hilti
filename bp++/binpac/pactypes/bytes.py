# $Id$
#
# The bytes type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.operator as operator

import hilti.core.type

@type.pac("bytes")
class Bytes(type.ParseableType):
    """Type for bytes objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Bytes, self).__init__(location=location)

    ### Overridden from Type.
    
    def name(self):
        return "bytes" 

    def validate(self, vld):
        # We need exactly one of the attributes. 
        c = 0
        for (name, const, default) in self._suppportedAttributes(self):
            if self.hasAttribute(name):
                c += 1
            
        if c == 0:
            vld.error(self, "bytes types needs an attribute")
            
        if c > 1:
            vld.error(self, "bytes types accepts exactly one attribute")

    def validateConst(self, vld, const):
        if not isinstance(const.value(), str):
            vld.error(const, "bytes: constant of wrong internal type")
            
    def hiltiType(self, cg):
        return hilti.core.type.Reference([hilti.core.type.Bytes()])

    def pac(self, printer):
        printer.output("bytes")
        
    def pacConstant(self, printer, value):
        printer.output("b\"%s\"" % value)
    
    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { 
            "length": (type.UnsignedInteger(64), False, None),
            "until": (type.Bytes(), False, None),
            }

    def production(self, field):
        return grammar.Variable(None, self, location=self.location())
    
    def generateParser(self, cg, cur, dst, skipping):
        
        bytesit = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        resultt = hilti.core.type.Tuple([self.hiltiType(cg), bytesit])
        
        def addTmp1():
            fbuilder = cg.functionBuilder()
            name = fbuilder._idName("end")
            return fbuilder.addLocal(name, bytesit)
        
        def addTmp2():
            fbuilder = cg.functionBuilder()
            name = fbuilder._idName("unpacked")
            return fbuilder.addLocal(name, resultt)
        
        # FIXME: We trust here that bytes iterators are inialized with the
        # generic end position. We should add a way to get that position
        # directly (but without a bytes object).
        end = cg.builder().cache(bytesit, addTmp1)
        
        op1 = cg.builder().tupleOp([cur, end])
        op2 = None
        op3 = None
        
        if self.hasAttribute("length"):
            op2 = cg.builder().idOp("Hilti::Packed::BytesFixed" if not skipping else "Hilti::Packed::SkipBytesFixed")
            expr = self.attributeExpr("length").castTo(type.UnsignedInteger(64), cg)
            op3 = expr.evaluate(cg)
            
        elif self.hasAttribute("until"):
            op2 = cg.builder().idOp("Hilti::Packed::BytesDelim" if not skipping else "Hilti::Packed::SkipBytesDelim")
            expr = self.attributeExpr("until").castTo(type.Bytes(), cg)
            op3 = expr.evaluate(cg)

        builder = cg.builder()
            
        result = builder.cache(str(resultt), addTmp2)

        builder.unpack(result, op1, op2, op3)
        
        if dst:
            builder.tuple_index(dst, result, builder.constOp(0))

        cur = builder.tuple_index(cur, result, builder.constOp(1))
        
        return cur
            
        
        
        
    
    
        
    
