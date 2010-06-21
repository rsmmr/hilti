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

    def validateInUnit(self, field, vld):
        # We need exactly one of the attributes. 
        c = 0
        for (name, (ty, const, default)) in self.supportedAttributes().items():
            if self.hasAttribute(name) and not name in ("default", "convert"):
                c += 1
            
        if c == 0:
            vld.error(field, "bytes type needs a termination attribute")
            
        if c > 1:
            vld.error(field, "bytes type accepts exactly one termination attribute")

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
            "convert": (type.Any(), False, None),
            "length": (type.UnsignedInteger(64), False, None),
            "until": (type.Bytes(), False, None),
            "eod": (None, False, None),
            }

    def production(self, field):
        filter = self.attributeExpr("convert")
        return grammar.Variable(None, self, filter=filter, location=self.location())

    def productionForLiteral(self, field, literal):
        filter = self.attributeExpr("convert")
        return grammar.Literal(None, literal, filter=filter)
    
    def fieldType(self):
        filter = self.attributeExpr("convert")
        if filter:
            return filter.type().resultType()
        else:
            return self.parsedType()
    
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
            expr = self.attributeExpr("length").coerceTo(type.UnsignedInteger(64), cg)
            op3 = expr.evaluate(cg)
            
        elif self.hasAttribute("until"):
            op2 = cg.builder().idOp("Hilti::Packed::BytesDelim" if not skipping else "Hilti::Packed::SkipBytesDelim")
            expr = self.attributeExpr("until").coerceTo(type.Bytes(), cg)
            op3 = expr.evaluate(cg)
            
        elif self.hasAttribute("eod"):
            
            loop = fbuilder.newBuilder("eod_loop")
            done = fbuilder.newBuilder("eod_reached")
            suspend = fbuilder.newBuilder("eod_not_reached")
            
            cg.builder().jump(loop.labelOp())
            
            eod = fbuilder.addTmp("__eod", hilti.type.Bool())
            loop.bytes_is_frozen(eod, cur)
            loop.if_else(eod, done.labelOp(), suspend.labelOp())
            
            cg.setBuilder(suspend)
            self.generateInsufficientInputHandler(cg, cur)
            cg.builder().jump(loop.labelOp())
            
            cg.setBuilder(done)
            if not skipping:
                done.bytes_sub(dst, cur, end)
            
            return end

        # FIXME: HILTI should have int64 lengths.
        op3_32= fbuilder.addTmp("__op32", hilti.type.Integer(32))
        cg.builder().int_trunc(op3_32, op3)
        
        result = self.generateUnpack(cg, op1, op2, op3_32)
        
        builder = cg.builder()
        
        if dst and not skipping:
            builder.tuple_index(dst, result, builder.constOp(0))

        builder.tuple_index(cur, result, builder.constOp(1))
        
        return cur
        
@operator.Size(Bytes)
class _:
    def type(e):
        return type.UnsignedInteger(64)
    
    def simplify(e):
        if e.isInit():
            n = len(e.value())
            return expr.Ctor(n, type.UnsignedInteger(64))
        
        else:
            return None
        
    def evaluate(cg, e):
        tmp = cg.functionBuilder().addLocal("__size", hilti.type.Integer(64))
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
        return expr.Ctor(b, type.Bool())
        
    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Plus(Bytes, Bytes)
class Plus:
    def type(e1, e2):
        return type.Bytes()

    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None
            
        b = (e1.value() + e2.value())
        return expr.Ctor(b, type.Bytes())
        
    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__copy", e1.type().hiltiType(cg))
        cg.builder().bytes_copy(tmp, e1.evaluate(cg))
        cg.builder().bytes_append(tmp, e2.evaluate(cg))
        return tmp
    
@operator.AddAssign(Bytes, Bytes)
class Plus:
    def type(e1, e2):
        return type.Bytes()

    def evaluate(cg, e1, e2):
        e1 = e1.evaluate(cg)
        cg.builder().bytes_append(e1, e2.evaluate(cg))
        return e1
    
@operator.MethodCall(type.Bytes, expr.Attribute("match"), [type.RegExp, operator.Optional(type.UnsignedInteger)])
class Match:
    def type(obj, method, args):
        return type.Bytes()
    
    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        re = args[0].evaluate(cg)
        
        if len(args) > 1:
            group = args[1].evaluate(cg)
        else:
            group = cg.builder().constOp(0)
        
        func = cg.builder().idOp("BinPACIntern::bytes_match")
        args = cg.builder().tupleOp([obj, re, group])
        tmp = cg.functionBuilder().addLocal("__match", obj.type())
        cg.builder().call(tmp, func, args)
        return tmp
    
@operator.MethodCall(type.Bytes, expr.Attribute("startswith"), [type.Bytes])
class Match:
    def type(obj, method, args):
        return type.Bool()
    
    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        pattern = args[0].evaluate(cg)
        
        func = cg.builder().idOp("Hilti::bytes_starts_with")
        args = cg.builder().tupleOp([obj, pattern])
        tmp = cg.functionBuilder().addLocal("__starts", hilti.type.Bool())
        cg.builder().call(tmp, func, args)
        return tmp
    
