# $Id$
#
# The unsigned integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.util as util
import binpac.operator as operator

import hilti.type

@type.pac("uint")
class UnsignedInteger(type.Integer):
    """Type for unsigned integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(UnsignedInteger, self).__init__(width, location=location)
        self._bits = None
        
    def setBits(self, bits):
        """Sets names for indvidual bits of this integer, which can then be
        used to access subvalues. 
        
        bits: dictionary string -> (int, int) - A dictionary mapping field names
        to the bit-range. (``(4,6)`` means bits 4 to (inclusive) 6; ``(1,1)``
        means just bit 1). 
        """
        self._bits = bits
        
    def bits(self):
        """Returns the bit fields the integer is broken down into, if set. 
        
        Returns: dictionary string -> (int, int), or None - A dictionary
        mapping field names to the bit-range. (``(4,6)`` means bits 4 to
        (inclusive) 6; ``(1,1)`` means just bit 1). 
        """
        return self._bits

    ### Overridden from node.Node.
        
    def _validate(self, vld):
        super(UnsignedIntegerBitField, self).__init(vld)
        for (lower, upper) in self._bits.values():
            if lower < 0 or upper >= self.width() or upper < lower:
                vld.error("invalid bit field specification (%d:%d)" % (lower, upper))

    ### Overridden from Type.
    
    def name(self):
        return "uint%d" % self.width()

    def hiltiType(self, cg):
        # If the byte-order is predetermined, our HILTI unpack format is fixed
        # and thus its result type matches what we're unpacking (except that
        # we have to extend the number of bytes because we don't have unsigned
        # values). 
        #
        # However, if our byte-order is not fixed, HILTI will always returns
        # an int64.
        if self._isByteOrderConstant():
            return hilti.type.Integer(min(64, self.width() * 2))
        else:
            return hilti.type.Integer(64)

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Integer(self.width()))
        return hilti.operand.Constant(const)

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { 
            "default": (self, True, None),
            "byteorder": (None, False, None),
            }

    def validateInUnit(self, field, vld):
        #if not self._getPackedEnum():
        #    vld.error("uint/byteorder combiniation not supported for parsing")
        pass
            
    def production(self, field):
        return grammar.Variable(None, self, location=self.location())

    def productionForLiteral(self, field, literal):
        util.internal_error("Type.productionForLiteral() not support for uint")
    
    def parsedType(self):
        return self
    
    def generateParser(self, cg, cur, dst, skipping):
        bytesit = hilti.type.IteratorBytes(hilti.type.Bytes())
        resultt = hilti.type.Tuple([self.hiltiType(cg), bytesit])
        fbuilder = cg.functionBuilder()
        
        # FIXME: We trust here that bytes iterators are inialized with the
        # generic end position. We should add a way to get that position
        # directly (but without a bytes object).
        end = fbuilder.addTmp("__end", bytesit)
        
        op1 = cg.builder().tupleOp([cur, end])
        
        if not skipping:
            op2 = self._hiltiByteOrderOp(cg)
            op3 = None
            assert op2
        else:
            assert self.width() in (8, 16, 32, 64)
            op2 = "Hilti::Packed::SkipBytesFixed"
            op3 = cg.builder().constOp(self.width() / 8, hilti.type.Integer(32))
            
        result = self.generateUnpack(cg, op1, op2, op3)
        
        builder = cg.builder()
        
        if dst and not skipping:
            builder.tuple_index(dst, result, builder.constOp(0))

        builder.tuple_index(cur, result, builder.constOp(1))
        
        return cur

    def bits(self):
        """Returns the bit fields the integer is broken down into.
        
        Returns: dictionary string -> (int, int) - A dictionary mapping field
        names to the bit-range. (``(4,6)`` means bits 4 to (inclusive) 6;
        ``(1,1)`` means just bit 1). 
        """
        return self._bits
        
    # Private.
     
    _packeds = {
        ("little", 8): "Hilti::Packed::UInt8Little",
        ("little", 16): "Hilti::Packed::UInt16Little",
        ("little", 32): "Hilti::Packed::UInt32Little",
        ("little", 64): "Hilti::Packed::Int64Little",
        ("big", 8): "Hilti::Packed::UInt8Big",
        ("big", 16): "Hilti::Packed::UInt16Big",
        ("big", 32): "Hilti::Packed::UInt32Big",
        ("big", 64): "Hilti::Packed::Int64Big",
        ("host", 8): "Hilti::Packed::UInt8",
        ("host", 16): "Hilti::Packed::UInt16",
        ("host", 32): "Hilti::Packed::UInt32",
        ("host", 64): "Hilti::Packed::Int64",
        ("network", 8): "Hilti::Packed::UInt8Big",
        ("network", 16): "Hilti::Packed::UInt16Big",
        ("network", 32): "Hilti::Packed::UInt32Big",
        ("network", 64): "Hilti::Packed::Int64Big",
        }
    
    def _isByteOrderConstant(self):
        expr = self._hiltiByteOrderExpr()
        return expr.isInit() if expr else True
        
    def _hiltiByteOrderExpr(self):
        order = self.attributeExpr("byteorder")
        if order:
            return order
        
        if self.field():
            prop = self.field().parent().property("byteorder", parent=self.field().parent().module())
            if prop:
                return prop
            
        return None

    def _hiltiByteOrderOp(self, cg):
        e = self._hiltiByteOrderExpr()

        if e and not e.isInit():
            # TODO: We need to add code here that selects the right unpack type
            # at run-time. 
            util.internal_error("non constant byte not yet supported")
        
        if e:
            assert isinstance(e, expr.Name)
            t = e.name().split("::")[-1].lower()
            
        else:
            # This is our default if no byte-order is specified. 
            t = "big"
            
        try:
            enum = UnsignedInteger._packeds[(t, self._width)]
        except KeyError:
            util.internal_error("unknown type/byteorder combination")
        
        return cg.builder().idOp(enum)
    
@operator.Coerce(type.UnsignedInteger)
class Coerce:
    def coerceCtorTo(ctor, dsttype):
        if isinstance(dsttype, type.Bool):
            return ctor != 0
        
        if isinstance(dsttype, type.SignedInteger):
            return ctor 
        
        raise operator.CoerceError

    def canCoerceExprTo(expr, dsttype):
        if isinstance(dsttype, type.Integer):
            return True
        
        if isinstance(dsttype, type.Bool):
            return True
        
        return False
        
    def coerceExprTo(cg, e, dsttype):
        if isinstance(dsttype, type.Bool):
            tmp = cg.builder().addLocal("__nonempty", hilti.type.Bool())
            cg.builder().equal(tmp, e.evaluate(cg), cg.builder().constOp(0))
            cg.builder().bool_not(tmp, tmp)
            return expr.Hilti(tmp, type.Bool())
        
        if dsttype.width() >= e.type().width():
            return e
        
        tmp = cg.builder().addLocal("__truncated", dsttype)
        cg.builder().int_trunc(tmp, e.evaluate(cg))
        return expr.Hilti(tmp, dsttype)
    
@operator.Plus(UnsignedInteger, UnsignedInteger)
class Plus:
    def type(expr1, expr2):
        return UnsignedInteger(max(expr1.type().width(), expr2.type().width()))
    
    def simplify(expr1, expr2):
        if isinstanct(expr1, expr.Ctor) and isinstanct(expr2, expr.Ctor):
            val = expr1.value() + expr2.value()
            return expr.Ctor(val, UnsignedInteger(max(expr1.type().width(), expr2.type().width())))

        else:
            return None
        
    def evaluate(cg, expr1, expr2):
        util.internal_error("not implemented")

@operator.Attribute(UnsignedInteger, type.String)
class Attribute:
    def validate(vld, lhs, ident):
        name = ident.name()
        if not lhs.type().bits():
            vld.error(lhs, "unknown attribute '%s'" % name)
        
        if name and not name in lhs.type().bits():
            vld.error(lhs, "unknown bitset field '%s'" % name)

    def type(lhs, ident):
        return lhs.type()

    def evaluate(cg, lhs, ident):
        name = ident.name() 
        builder = cg.builder()
        (lower, upper) = lhs.type().bits()[name]
        tmp = builder.addLocal("__bits", lhs.type().hiltiType(cg))
        cg.builder().int_mask(tmp, lhs.evaluate(cg), builder.constOp(lower), builder.constOp(upper))
        return tmp

    def assign(cg, lhs, ident, rhs):
        util.internal_error("assigning to bitfields is not support currently")
    
