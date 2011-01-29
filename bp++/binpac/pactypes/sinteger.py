# $Id$
#
# The signed integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.util as util
import binpac.operator as operator

import hilti.type

@type.pac("int")
class SignedInteger(type.Integer):
    """Type for signed integers.

    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(SignedInteger, self).__init__(width, location=location)

    ### Overidden from Integer.

    def _typeOfWidth(self, n):
        return SignedInteger(n)

    def _usableBits(self):
        return self.width() - 1

    def _packEnums(self):
        return {
            ("little", 8): "Hilti::Packed::Int8Little",
            ("little", 16): "Hilti::Packed::Int16Little",
            ("little", 32): "Hilti::Packed::Int32Little",
            ("little", 64): "Hilti::Packed::Int64Little",
            ("big", 8): "Hilti::Packed::Int8Big",
            ("big", 16): "Hilti::Packed::Int16Big",
            ("big", 32): "Hilti::Packed::Int32Big",
            ("big", 64): "Hilti::Packed::Int64Big",
            ("host", 8): "Hilti::Packed::Int8",
            ("host", 16): "Hilti::Packed::Int16",
            ("host", 32): "Hilti::Packed::Int32",
            ("host", 64): "Hilti::Packed::Int64",
            ("network", 8): "Hilti::Packed::Int8Big",
            ("network", 16): "Hilti::Packed::Int16Big",
            ("network", 32): "Hilti::Packed::Int32Big",
            ("network", 64): "Hilti::Packed::Int64Big",
            }

    def _range(self):
        t = 2 ** (self.width() - 1)
        return (-t, t - 1)

    def _ext(self, cg, target, val):
        cg.builder().int_sext(target, val)

    def _lt(self, cg, target, val1, val2):
        cg.builder().int_slt(target, val1, val2)

    def _leq(self, cg, target, val1, val2):
        cg.builder().int_sleq(target, val1, val2)

    def _gt(self, cg, target, val1, val2):
        cg.builder().int_sgt(target, val1, val2)

    def _geq(self, cg, target, val1, val2):
        cg.builder().int_sgeq(target, val1, val2)

    def _shr(self, cg, target, val1, val2):
        cg.builder().int_ashr(target, val1, val2)

    ### Overridden from Type.

    def name(self):
        return "int%d" % self.width()

