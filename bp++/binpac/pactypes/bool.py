# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("bool")
class Bool(type.ParseableType):
    """Type for bool objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Bool, self).__init__(location=location)

    ### Overridden from Type.

    def validate(self, vld):
        return True

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, bool):
            vld.error(ctor, "bool: constant of wrong internal type")

    def hiltiType(self, cg):
        return hilti.type.Bool()

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Bool())
        return hilti.operand.Constant(const)

    def name(self):
        return "bool"

    def pac(self, printer):
        printer.output("bool")

    def pacCtor(self, printer, value):
        printer.output("True" if value else "False")

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self, field):
        util.internal_error("bool parsing not implemented")

    def generateParser(self, cg, var, args, dst, skipping):
        util.internal_error("bool parsing not implemented")

@operator.LogicalAnd(Bool, Bool)
class _:
    def type(e1, e2):
        return type.Bool()

    def simplify(e1, e2):
        if not (isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor)):
            return None

        result = e1.value() and e2.value()
        return expr.Ctor(result, type.Bool())

    def evaluate(cg, e1, e2):
        fbuilder = cg.functionBuilder()

        result = fbuilder.addLocal("__and", hilti.type.Bool())
        op = fbuilder.addLocal("__op", hilti.type.Bool())

        # We (must) short-cut the evaluation if the first operand already
        # returns false.

        eval_op2 = fbuilder.newBuilder("and_eval_op2")
        true = fbuilder.newBuilder("and_true")
        false = fbuilder.newBuilder("and_false")
        cont = fbuilder.newBuilder("and_cont")

        op = e1.evaluate(cg)
        cg.builder().if_else(op, eval_op2, false)

        cg.setBuilder(eval_op2)
        op = e2.evaluate(cg)
        cg.builder().if_else(op, true, false)

        true.assign(result, true.constOp(True))
        true.jump(cont)

        false.assign(result, false.constOp(False))
        false.jump(cont)

        cg.setBuilder(cont)
        return result

@operator.LogicalOr(Bool, Bool)
class _:
    def type(e1, e2):
        return type.Bool()

    def simplify(e1, e2):
        if not (isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor)):
            return None

        result = e1.value() or e2.value()
        return expr.Ctor(result, type.Bool())

    def evaluate(cg, e1, e2):
        fbuilder = cg.functionBuilder()

        result = fbuilder.addLocal("__or", hilti.type.Bool())
        op = fbuilder.addLocal("__op", hilti.type.Bool())

        # We (must) short-cut the evaluation if the first operand already
        # returns true.

        eval_op2 = fbuilder.newBuilder("or_eval_op2")
        true = fbuilder.newBuilder("or_true")
        false = fbuilder.newBuilder("or_false")
        cont = fbuilder.newBuilder("or_cont")

        op = e1.evaluate(cg)
        cg.builder().if_else(op, true, eval_op2)

        cg.setBuilder(eval_op2)
        op = e2.evaluate(cg)
        cg.builder().if_else(op, true, false)

        true.assign(result, true.constOp(True))
        true.jump(cont)

        false.assign(result, false.constOp(False))
        false.jump(cont)

        cg.setBuilder(cont)
        return result




