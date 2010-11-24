# $Id$
#
# Generic iterator operators.

import binpac.type as type
import binpac.operator as operator

@operator.Deref(type.Iterator)
class _:
    def type(iter):
        return iter.type().derefType()

    def evaluate(cg, iter):
        tmp = cg.functionBuilder().addLocal("__elem", iter.type().derefType().hiltiType(cg))
        cg.builder().deref(tmp, iter.evaluate(cg))
        return tmp

@operator.IncrPrefix(type.Iterator)
class _:
    def type(iter):
        return iter.type()

    def evaluate(cg, iter):
        op = iter.evaluate(cg)
        cg.builder().incr(op, op)
        return op

@operator.IncrPostfix(type.Iterator)
class _:
    def type(iter):
        return iter.type()

    def evaluate(cg, iter):
        tmp = cg.functionBuilder().addLocal("__tmp", iter.type().hiltiType(cg))
        op = iter.evaluate(cg)
        cg.builder().assign(tmp, op)
        cg.builder().incr(op, op)
        return tmp

@operator.PlusAssign(type.Iterator, type.UnsignedInteger)
class _:
    def type(iter, incr):
        return iter.type()

    def evaluate(cg, iter, incr):
        op1 = iter.evaluate(cg)
        op2 = incr.evaluate(cg)
        cg.builder().incr_by(op1, op1, op2)
        return op1

@operator.Plus(type.Iterator, type.UnsignedInteger)
class _:
    def type(iter, incr):
        return iter.type()

    def evaluate(cg, iter, incr):
        op1 = iter.evaluate(cg)
        op2 = incr.coerceTo(type.UnsignedInteger(64), cg).evaluate(cg)
        tmp = cg.functionBuilder().addLocal("__niter", op1.type())
        cg.builder().incr_by(tmp, op1, op2)
        return tmp

@operator.Equal(type.Iterator, type.Iterator)
class _:
    def type(iter1, iter2):
        return type.Bool()

    def evaluate(cg, iter1, iter2):
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, iter1.evaluate(cg), iter2.evaluate(cg))
        return tmp




