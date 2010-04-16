# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.constant as constant
import binpac.grammar as grammar
import binpac.operator as operator

import hilti.type
import hilti.operand

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

    def validateInUnit(self, vld):
        # We need exactly one of the attributes. 
        c = 0
        for (name, (ty, const, default)) in self.supportedAttributes().items():
            if self.hasAttribute(name) and name != "default":
                c += 1
            
        if c == 0:
            vld.error(self, "bytes type needs a termination attribute")
            
        if c > 1:
            vld.error(self, "bytes type accepts exactly one termination attribute")

    def validateCtor(self, vld, value):
        if not isinstance(value, str):
            vld.error(const, "bytes: ctor of wrong internal type")

    def hiltiCtor(self, cg, val):
        return hilti.operand.Ctor(val, hilti.type.Reference(hilti.type.Bytes()))
            
    def hiltiType(self, cg):
        return hilti.type.Reference(hilti.type.Bytes())

    def hiltiDefault(self, cg, must_have=True):
        if not must_have:
            return None
        
        return hilti.operand.Ctor("", self.hiltiType(cg))
    
    def pac(self, printer):
        printer.output("bytes")
        
    def pacCtor(self, printer, value):
        printer.output("b\"%s\"" % value)
    
    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { 
            "default": (self, True, None),
            "length": (type.UnsignedInteger(64), False, None),
            "until": (type.Bytes(), False, None),
            }

    def production(self, field):
        return grammar.Variable(None, self, location=self.location())
    
    def generateParser(self, cg, cur, dst, skipping):
        
        bytesit = hilti.type.IteratorBytes(hilti.type.Bytes())
        resultt = hilti.type.Tuple([self.hiltiType(cg), bytesit])
        fbuilder = cg.functionBuilder()
        
        # FIXME: We trust here that bytes iterators are inialized with the
        # generic end position. We should add a way to get that position
        # directly (but without a bytes object).
        end = fbuilder.addTmp("__end", bytesit)
        
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
            
        result = fbuilder.addTmp("__unpacked", resultt)
        builder.unpack(result, op1, op2, op3)
        
        if dst and not skipping:
            builder.tuple_index(dst, result, builder.constOp(0))

        builder.tuple_index(cur, result, builder.constOp(1))
        
        return cur
        
@operator.Size(Bytes)
class _:
    def type(e):
        return type.UnsignedInteger(32)
    
    def simplify(e):
        if e.isInit():
            n = len(e.value())
            const = constant.Constant(n, type.UnsignedInteger(32))
            return expr.Constant(const)
        
        else:
            return None
        
    def evaluate(cg, e):
        tmp = cg.functionBuilder().addTmp("__size", hilti.type.Integer(32))
        cg.builder().bytes_length(tmp, e.evaluate(cg))
        return tmp
    
@operator.Equal(Bytes, Bytes)
class _:
    def type(e1, e2):
        return type.Bool()
    
    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None
            
        b = (e1.value() == e2.value())
        return expr.Constant(constant.Constant(b, type.Bool()))
        
    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addTmp("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp
