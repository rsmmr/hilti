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

    ### Overidden from Integer.

    def _typeOfWidth(self, n):
        return UnsignedInteger(n)

    def _usableBits(self):
        return self.width()

    def _packEnums(self):
        return {
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
            ("network", 64): "Hilti::Packed::UInt64Big",
            }

    def _range(self):
        return (0, 2**self.width() - 1)

    def _ext(self, cg, target, val):
        cg.builder().int_zext(target, val)

    def _lt(self, cg, target, val1, val2):
        cg.builder().int_ult(target, val1, val2)

    def _leq(self, cg, target, val1, val2):
        cg.builder().int_uleq(target, val1, val2)

    def _gt(self, cg, target, val1, val2):
        cg.builder().int_ugt(target, val1, val2)

    def _geq(self, cg, target, val1, val2):
        cg.builder().int_ugeq(target, val1, val2)

    def _shr(self, cg, target, val1, val2):
        cg.builder().int_shr(target, val1, val2)

    ### Overridden from Type.

    def name(self):
        return "uint%d" % self.width()

@operator.Power(UnsignedInteger, UnsignedInteger)
class _:
    def type(e1, e2):
        return integer._checkTypes(e1, e2)

    def evaluate(cg, e1, e2):
        (e1, e2) = integer._extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__pow", hilti.type.Integer(e1.type().width()))
        cg.builder().int_pow(tmp, e1, e2)
        return tmp
