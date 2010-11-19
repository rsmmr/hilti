# $Id$
#
# The tuple type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac(None)
class Tuple(type.Type):
    """Type for tuple objects.

    types: list of ~~Type - The types of the individual tuple elements; must
    not be empty.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, args, location=None):
        super(Tuple, self).__init__(location=location)
        assert args
        self._types = args

    def types(self):
        """Returns the types of the typle elements.

        Returns: list of ~~Type - The types.
        """
        return self._types

    ### Overridden from Type.

    def validate(self, vld):
        for t in self._types:
            t.validate(vld)

    def validateCtor(self, vld, ctor):

        if len(ctor) != len(self._types):
            vld.error(ctor, "tuple length does not match type")

        for (c, t) in zip(ctor, self._types):
            t.validateCtor(vld, c.value())

    def hiltiType(self, cg):
        return hilti.type.Tuple([t.hiltiType(cg) for t in self._types])

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant([c.type().hiltiCtor(cg, c.value()) for c in ctor], self.hiltiType(cg))
        return hilti.operand.Constant(const)

    def name(self):
        return "tuple<%s>" % (",".join([t.name() for t in self._types]))

    def pac(self, printer):
        printer.output(self.name())

    def pacCtor(self, printer, value):
        printer.output("(%s)" % ",".join(t.pacCtor(v) for v in value))

def _canCoerceTo(e1, e2):
    if not isinstance(e1.type(), type.Tuple):
        return False

    if not isinstance(e2.type(), type.Tuple):
        return False

    if len(e1.type().types()) != len(e1.type().types()):
        return False

    for (t1, t2) in zip(e1.type().types(), e2.type().types()):
        if not t1.canCoerceTo(t2):
            return False

    return True

@operator.Equal(type.Tuple, type.Tuple)
class _:
    def type(e1, e2):
        return type.Bool()

    def validate(vld, e1, e2):
        if not _canCoerceTo(e1, e2) and not _canCoerceTo(e2, e1):
            vld.error("tuple types don't match")

    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())

        # We cheat here and rely on HILTI to do the coercion right. Otherwise,
        # we'd have to split the tuple apart, coerce each element individually,
        # and then put it back together; something which is probably always
        # unnecessary.

        if _canCoerceTo(e1, e2):
            cg.builder().equal(tmp, e1.coerceTo(e2.type(), cg).evaluate(cg), e2.evaluate(cg))
        else:
            cg.builder().equal(tmp, e1.evaluate(cg), e2.coerceTo(e2.type(), cg).evaluate(cg))

        return tmp

@operator.Index(type.Tuple, type.Integer)
class _:
    def type(t, i):
        return t.type().types()[i.value()]

    def validate(vld, t, i):
        if not i.isInit():
            vld.error("tuple index must be constant")

        if i.value() < 0 or i.value() > len(t.type().types()):
            vld.error("tuple index out of range")

        t.validate(vld)

    def evaluate(cg, t, i):
        et = t.type().types()[i.value()]
        tmp = cg.functionBuilder().addLocal("__elem", et.hiltiType(cg))
        cg.builder().tuple_index(tmp, t.evaluate(cg), i.evaluate(cg))
        return tmp
