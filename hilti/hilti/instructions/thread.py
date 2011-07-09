# $Id$
"""

.. hlt:type:: thread

   XXX
"""

builtin_id = id

import llvm.core

import hilti.block as block
import hilti.function as function
import hilti.operand as operand

import operators
import flow
import struct

from hilti.instruction import *
from hilti.constraints import *

@hlt.type(None, 7) # Same type as struct.
class Context(type.Struct):
    """Type for a threading ``context``.

    fields: list of ~~ID - The fields of the context.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, fields, location=None):
        # Sort the fields so that order doesn't matter for the hashing.
        fields.sort(key=lambda f: f.name())

        # Add an internal field with a constant hash that depends on the
        # defined fields. This forces different scheduling for contexts that
        # aren't fully equivalent.
        def simple_hash(s):
            return reduce(lambda x,y: x+y, [ord(c) for c in s])

        n = 0
        for f in fields:
            n += simple_hash(f.name()) + simple_hash(str(f.type()))

        self._hash = n

        hash_id = id.Local("__type_hash", type.Integer(64))
        hash_val = operand.Constant(constant.Constant(n, hash_id.type()))
        hash_id.setLinkage(id.Linkage.INTERNAL)

        fields = [(f, None) for f in fields]
        fields += [(hash_id, hash_val)]

        # Call standard struct ctor.
        super(Context, self).__init__(fields, location=location)

    def fields(self):
        """Returns the context's fields.

        Returns: list of ~~ID - The context's fields.
        """
        return [i for (i, op) in super(Context, self).fields()]

    def output(self, printer):
        printer.output("{", nl=True)
        printer.push()
        first = True
        for i in self.fields():
            if i.linkage() == id.Linkage.INTERNAL:
                continue

            if not first:
                printer.output(",", nl=True)
            i.output(printer)
            first = False

        printer.output("", nl=True)
        printer.pop()
        printer.output("}", nl=True)

# Sentinel bound to the global "context" until defines a context.
NoContext = Context([])

@hlt.type(None, 36)
class Scope(type.HiltiType):
    """Type for a threading ``scope``.

    fields: list of string - The field names of the context that are part of
    this scope.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, fields, location=None):
        super(Scope, self).__init__(location=location)
        self._fields = fields

    def fields(self):
        """Returns the fields that make up the scope.

        Returns: list of string - The field names.
        """
        return self._fields

    def cmpWithSameType(self, other):
        return self._fields == other._fields

    def output(self, printer):
        printer.output("{ ")
        first = True
        for f in self.fields():
            if not first:
                printer.output(", ")
            printer.output(f)
            first = False
        printer.output(" }", nl=True)

# Returns a dictionary of all IDs that make up a scope, extraced from a
# context and indexed by field name.
def _scopeIDs(scope, context):
    ids = [(f, context.field(f)) for f in scope.fields()]

    assert not (None in ids)

    return dict(ids)

# Checks whether Operand *op* can be used to initialize a thread context of the
# given scope.
def _checkScopeInit(op, scope, context):
    # Make sure the tuple's types match the context struct.
    if not op.canCoerceTo(type.Reference(context)):
        return False

    # TODO: If we get a dynamic struct/context, should we check that all fields
    # needed by the scope are set?

    if isinstance(op.type(), type.Tuple):
        if isinstance(op, operand.Constant):
            # Match sure we set all fields that our scope requires.
            ids = _scopeIDs(scope, context)
            for (field, ty) in zip(context.fields(), op.type().types()):
                if isinstance(ty, type.Tuple.NotSet) and field in ids:
                    return False
        else:
            # I think this can't happen ...
            util.internal_error("run-time scope type check for dynamic tuples not implemented")

    return True

# Checks whether we can promote from a source scope to a destination scope.
def _checkContextPromotion(src_scope, src_context, dst_scope, dst_context):
    src_ids = _scopeIDs(src_scope, src_context)
    dst_ids = _scopeIDs(dst_scope, dst_context)

    for (d_name, d_type) in dst_ids.items():
        try:
            s_type = src_ids[d_name]
        except KeyError:
            return False

        if str(s_type) != str(d_type):
            return False

    return True

# Checks whether we can *call* dst_func from src_func.
def _checkCall(src_func, dst_func):
    src_scope = src_func.threadScope()
    src_context = src_func.scope().lookup("context").type()
    dst_scope = dst_func.threadScope()
    dst_context = dst_func.scope().lookup("context").type()

    if not dst_scope:
        return true

    return src_scope == dst_scope and src_context == dst_context

# Promotes thread context from a source scope to destination scope.
# _checkContextPromotion() has already verified that this operation is to ok to
# do. Note that we must never modify an existing context. We can directly
# reuse it however if that does not require any modification.
def _promoteContext(cg, tcontext, src_scope, src_context, dst_scope, dst_context):
    # If both scope and context match, we don't need to do anything.
    if src_scope == dst_scope and src_context == dst_context:
        return tcontext

    dst_ids = _scopeIDs(dst_scope, dst_context)

    tcontext = cg.builder().bitcast(tcontext, cg.llvmType(src_context))

    # Build a list of field values for an instance of the destination context.

    vals = []
    fields = [i.name() for i in dst_context.fields()]

    for name in fields:
        if name in dst_ids:
            # Copy the field over from the source context.
            vals += [src_context.llvmGetField(cg, tcontext, src_context.fieldIndex(name))]
        else:
            # Scope doesn't need it, just leave unset.
            vals += [type.Struct.NotSet()]

    return dst_context.llvmFromValues(cg, vals)

@hlt.instruction("thread.schedule", op1=cFunction, op2=cTuple, op3=cOptional(cIntegerOfWidth(64)), terminator=False)
class ThreadSchedule(Instruction):
    """Schedules a function call onto a virtual thread. If *op3* is
    not given, the current thread context determines the target
    thread, according to HILTI context-based scheduling. If *op3* is
    given, it gives the target thread ID directly; in this case the
    functions thread context will be cleared when running.
    """
    def _validate(self, vld):
        super(ThreadSchedule, self)._validate(vld)

        if vld.errors() > 0:
            return

        func = self.op1().value().function()
        if not flow._checkFunc(vld, self, func, [function.CallingConvention.HILTI], schedule=True):
            return

        rt = func.type().resultType()

        if rt != type.Void:
            vld.error(self, "thread.schedule used to call a function that returns a value")

        flow._checkArgs(vld, self, func, self.op2())

        if not self.op3():
            src_scope = vld.currentFunction().threadScope()
            src_context = vld.currentFunction().scope().lookup("context").type()
            dst_scope = func.threadScope()
            dst_context = func.scope().lookup("context").type()

            if builtin_id(src_context) == builtin_id(NoContext):
                vld.error(self, "current module does not define a thread context")
                return

            if not src_scope:
                vld.error(self, "current function does not define a scope")
                return

            if builtin_id(dst_context) == builtin_id(NoContext):
                vld.error(self, "callee function's module does not define a thread context")
                return

            if not dst_scope:
                vld.error(self, "callee function does not define a scope")
                return

            if not _checkContextPromotion(src_scope.type(), src_context, dst_scope.type(), dst_context):
                vld.error(self, "scope of callee function is incompatible with the current scope")

    def _codegen(self, cg):
        func = cg.lookupFunction(self.op1())

        args = self.op2()
        cont = cg.llvmMakeCallable(func, args)

        mgr = cg.llvmGlobalThreadMgrPtr()
        ctx = cg.llvmCurrentExecutionContextPtr()

        null_tcontext = llvm.core.Constant.null(cg.llvmTypeGenericPointer())

        if self.op3():
            vid = self.op3().coerceTo(cg, type.Integer(64))
            vid = cg.llvmOp(vid)
            cg.llvmCallCInternal("__hlt_thread_mgr_schedule", [mgr, vid, cont, cg.llvmFrameExceptionAddr(), ctx])
            cg.llvmExceptionTest()

        else:
            tcontext = cg.llvmExecutionContextThreadContext()

            # Ensure that we have a context set.
            block_ok = cg.llvmNewBlock("ok")
            block_exc = cg.llvmNewBlock("exc")

            isnull = cg.builder().icmp(llvm.core.IPRED_NE, tcontext, null_tcontext)
            cg.builder().cbranch(isnull, block_ok, block_exc)

            cg.pushBuilder(block_exc)
            cg.llvmRaiseExceptionByName("hlt_exception_no_thread_context", self.location())
            cg.popBuilder()

            cg.pushBuilder(block_ok)

            # Do the Promotion.
            src_scope = cg.currentFunction().threadScope().type()
            src_context = cg.currentFunction().scope().lookup("context").type()
            dst_scope = func.threadScope().type()
            dst_context = func.scope().lookup("context").type()

            tcontext = _promoteContext(cg, tcontext, src_scope, src_context, dst_scope, dst_context)

            ti = cg.builder().bitcast(cg.llvmTypeInfoPtr(dst_context), cg.llvmTypeTypeInfoPtr())
            tcontext = cg.builder().bitcast(tcontext, cg.llvmTypeGenericPointer())
            cg.llvmCallCInternal("__hlt_thread_mgr_schedule_tcontext", [mgr, ti, tcontext, cont, cg.llvmFrameExceptionAddr(), ctx])
            cg.llvmExceptionTest()

            # Leave ok-builder for subsequent code.


@hlt.instruction("thread.id", target=cIntegerOfWidth(64))
class ThreadID(Instruction):
    """Returns the ID of the current virtual thread. Returns -1 if executed in
    the primary thread.
    """
    def _codegen(self, cg):
        cg.llvmStoreInTarget(self, cg.llvmExecutionContextVThreadID())

@hlt.instruction("thread.set_context", op1=cHiltiType)
class SetContext(Instruction):
    """Sets the thread context of the currently executing virtual thread to
    *op1*. The type of *op1* must match the current module's
    ``context`` definition.
    """

    def validate(self, vld):
        super(SetContext, self)._validate(vld)

        context = vld.currentFunction().scope().lookup("context").type()

        if builtin_id(context) == builtin_id(NoContext):
            vld.error(self, "module does not define a thread context")
            return

        if not self.op1().canCoerceTo(type.Reference(context)):
            vld.error(self, "op1 must be of type %s but is %s" % (context, self.op1().type()))
            return

        scope = vld.currentFunction().threadScope()

        if not scope:
            vld.error(self, "current function does not have a scope defined")

        elif not _checkScopeInit(self.op1(), scope.type(), vld.currentModule().threadContext()):
            vld.error(self, "set of scope fields does not match current function's scope")

    def _codegen(self, cg):
        context = cg.currentFunction().scope().lookup("context").type()
        op1 = self.op1().coerceTo(cg, type.Reference(context))
        op1 = cg.llvmOp(op1)
        cg.llvmExecutionContextSetThreadContext(op1)

@hlt.instruction("thread.get_context", target=cReferenceOf(cContext))
class GetContext(Instruction):
    """Returns the thread context of the currently executing virtual thread.
    The type of the result operand must opf type ``context``, which is
    implicitly defined as current function's thread context type. The
    instruction cannot be used if no such has been defined.
    """
    def _validate(self, vld):
        super(GetContext, self)._validate(vld)

        context = vld.currentFunction().scope().lookup("context").type()

        if builtin_id(context) == builtin_id(NoContext):
            vld.error(self, "module does not define a thread context")
            return

        if not self.target().canCoerceTo(type.Reference(context)):
            vld.error(self, "result must be of type ref<%s> but is %s" % (context, self.target().type()))
            return

    def _codegen(self, cg):
        context = cg.currentFunction().scope().lookup("context").type()

        tcontext = cg.llvmExecutionContextThreadContext()
        tcontext = cg.builder().bitcast(tcontext, cg.llvmType(context))
        cg.llvmStoreInTarget(self, tcontext)
