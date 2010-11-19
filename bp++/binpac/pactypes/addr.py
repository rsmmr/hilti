# $Id$
#
# The addr type.

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.operator as operator

import hilti.type
import hilti.operand

@type.pac("addr")
class Addr(type.ParseableWithByteOrder):
    """Type for address objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Addr, self).__init__(location=location)

    ### Overridden from Type.

    def name(self):
        return "addr"

    def validateInUnit(self, field, vld):
        # We need either the v4 or the v6 attribute.
        v4 = self.hasAttribute("ipv4")
        v6 = self.hasAttribute("ipv6")

        if not v4 and not v6:
            vld.error(field, "need either &ipv4 or &ipv6 attribute")

        if v4 and v6:
            vld.error(field, "cannot have both &ipv4 and &ipv6 attribute")

    def validateCtor(self, vld, value):
        # Todo: implement.
        pass

    def hiltiCtor(self, cg, val):
        return hilti.operand.Constant(hilti.constant.Constant(val, hilti.type.Addr()))

    def hiltiType(self, cg):
        return hilti.type.Addr()

    def pac(self, printer):
        printer.output("addr")

    def pacCtor(self, printer, value):
        # Not implemented yet.
        assert False

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {
            "byteorder": ("binpac::ByteOrder", False, None),
            "ipv4": (None, False, None),
            "ipv6": (None, False, None),
            }

    def production(self, field):
        return grammar.Variable(None, self, location=self.location())

    _packeds = {
        ("host", True): "Hilti::Packed::IPv4",
        ("host", False): "Hilti::Packed::IPv6",
        ("network", True): "Hilti::Packed::IPv4Network",
        ("network", False): "Hilti::Packed::IPv6Network",
        ("big", True): "Hilti::Packed::IPv4Network",
        ("big", False): "Hilti::Packed::IPv6Network"
        # FIXME: Don't have a HILTI enum for addr in little-endian.
        }

    def generateParser(self, cg, cur, dst, skipping):
        resultt = hilti.type.Tuple([hilti.type.Addr()])
        fbuilder = cg.functionBuilder()

        v4 = self.hasAttribute("ipv4")
        v6 = self.hasAttribute("ipv6")

        # FIXME: We trust here that bytes iterators are inialized with the
        # generic end position. We should add a way to get that position
        # directly (but without a bytes object).
        end = fbuilder.addTmp("__end", hilti.type.IteratorBytes())

        op1 = cg.builder().tupleOp([cur, end])

        if not skipping:
            op2 = self._hiltiByteOrderOp(cg, Addr._packeds, v4)
            op3 = None
            assert op2
        else:
            op2 = cg.builder().idOp("Hilti::Packed::SkipBytesFixed")
            op3 = cg.builder().constOp(4 if v4 else 16, hilti.type.Integer(32))

        result = self.generateUnpack(cg, op1, op2, op3)

        builder = cg.builder()

        if dst and not skipping:
            builder.tuple_index(dst, result, builder.constOp(0))

        builder.tuple_index(cur, result, builder.constOp(1))

        return cur

@operator.Equal(Addr, Addr)
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

_addr_family = type.Unknown("binpac::AddrFamily")

@operator.MethodCall(Addr, expr.Attribute("family"), [])
class Family:
    def type(obj, method, args):
        return _addr_family

    def resolve(resolver, obj, method, args):
        global _addr_family
        if isinstance(_addr_family, type.Unknown):
            _addr_family = resolver.scope().lookupID(_addr_family.idName()).type()

    def evaluate(cg, obj, method, args):
        ty = cg.builder().idOp("Hilti::AddrFamily").type()
        tmp = cg.functionBuilder().addLocal("__family", ty)
        cg.builder().addr_family(tmp, obj.evaluate(cg))
        return tmp


