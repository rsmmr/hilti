# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.id as id
import binpac.operator as operator

import hilti.type

@type.pac("enum")
class Enum(type.ParseableType):
    """Type for enumeration values. In addition to the specificed labels, an
    additional ``Undef`` value is implicitly defined and is the default value
    for all not otherwise initialized values.

    labels: list of tuples (strings, value) - The labels, with their
    associated values. If a value is None (not zero!), it will be assigned
    automatically.

    scope: ~~Scope - Scope into which the labels are inserted.

    namesapce: string  - The namespace for the labels (i.e., the name of the
    enum type).

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, labels, scope, namespace, location=None):
        super(Enum, self).__init__(location=location)
        self._scope = scope
        self._namespace = namespace

        last_val = 1
        self._labels = {}

        for (name, val) in labels:
            next_val = val if val != None else last_val + 1
            self._labels[name] = next_val
            last_val = next_val

        self._hlttype = None

        for label in self._labels.keys() + ["Undef"]:
            eid = id.Constant(namespace + "::" + label, self, expr.Ctor(label, self))
            scope.addID(eid)

    ### Overridden from Type.

    def validate(self, vld):
        if "Undef" in self._labels:
            vld.error(self, "Undef enum value already predefined.")

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, str):
            vld.error(ctor, "enum: ctor of wrong internal type")

        if not ctor in self._labels.keys() + ["Undef"]:
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
        idents = ["%s = %d" % (name, val) for (name, val) in self._labels.items()]
        printer.output("enum { %s }" % ", ".join(sorted(idents)))

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


# We allow an enum type to be used as a function call with an uint
# argument. The call will convert the integer into an enum value. This can
# in particular be used with the &convert attribute.
@operator.Call(type.Enum, [type.UnsignedInteger])
class Call:
    def type(t, args):
        return t.type()

    def evaluate(cg, t, args):
        val = args[0]
        tmp = cg.functionBuilder().addLocal("__enum", t.type().hiltiType(cg))

        if val.type().width() < 64:
            ext = cg.functionBuilder().addLocal("__ext", hilti.type.Integer(64))
            cg.builder().int_zext(ext, val.evaluate(cg))
            val = ext
        else:
            val = val.evaluate(cg)

        cg.builder().enum_from_int(tmp, val)
        return tmp
