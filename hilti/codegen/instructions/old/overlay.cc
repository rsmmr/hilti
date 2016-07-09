
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::overlay::Attach* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            overlayt = self.op1().type()

            # Save the starting offset.
            ov = cg.llvmOp(self.op1())
            iter = cg.llvmOp(self.op2())
            ov = cg.llvmInsertValue(ov, 0, iter)

            # Clear the cache
            end = bytes.llvmEnd(cg)
            for j in range(1, overlayt._arraySize()):
                ov = cg.llvmInsertValue(ov, j, end)

            # Need to rewrite back into the original value.
            self.op1().llvmStore(cg, ov)

    */
}

void StatementBuilder::visit(statement::instruction::overlay::Get* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);

        auto result = builder()->

        cg()->llvmStore(i, result);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());

        auto result = cg()->llvmCall("hlt::X", args);

        cg()->llvmStore(i, result);
    */


    /*
        def _codegen(self, cg):
            overlayt = self.op1().type()
            ov = cg.llvmOp(self.op1())
            offset0 = cg.llvmExtractValue(ov, 0)

            # Generate the unpacking code for one field, assuming all dependencies are
            # already resolved.
            def _emitOne(ov, field):
                offset = field.offset()

                if offset == 0:
                    # Starts right at the beginning.
                    begin = offset0

                elif offset > 0:
                    # Static offset. We can calculate the starting position directly.
                    arg1 = operand.LLVM(offset0, type.IteratorBytes())
                    arg2 = operand.Constant(constant.Constant(offset, type.Integer(32)))
                    begin = cg.llvmCallC("hlt::bytes_pos_incr_by", [arg1, arg2])

                else:
                    # Offset is not static but our immediate dependency must have
                    # already been calculated at this point so we take it's end
                    # position out of the cache.
                    deps = field.dependants()
                    assert deps
                    dep = overlayt.field(deps[-1]).depIndex()
                    assert dep > 0
                    begin = cg.llvmExtractValue(ov, dep)

                if field.arg():
                    arg = cg.llvmOp(field.arg())
                    arg = operand.LLVM(arg, field.arg().type())
                else:
                    arg = None

                (val, end) = cg.llvmUnpack(field.type(), begin, None, field.fmt(), arg)

                if field.depIndex() >= 0:
                    ov = cg.llvmInsertValue(ov, field.depIndex(), end)

                return (ov, val)

            # Generate code for all depedencies.
            def _makeDep(ov, field):

                if not field.dependants():
                    return ov

                # Need a tmp or a PHI node. LLVM's mem2reg should optimize the tmp
                # away so this is easier.
                # FIXME: Check that mem2reg does indeed.
                new_ov = cg.llvmAlloca(ov.type)
                dep = overlayt.field(field.dependants()[-1])
                block_cont = cg.llvmNewBlock("have-%s" % dep.name)
                block_cached = cg.llvmNewBlock("cached-%s" % dep.name)
                block_notcached = cg.llvmNewBlock("not-cached-%s" % dep.name)

                _isEnd(cg, ov, dep.depIndex(), block_notcached, block_cont)

                cg.pushBuilder(block_notcached)
                ov = _makeDep(ov, dep)
                cg.builder().store(ov, new_ov)
                cg.builder().branch(block_cont)
                cg.popBuilder()

                cg.pushBuilder(block_cached)
                cg.builder().store(ov, new_ov)
                cg.builder().branch(block_cont)
                cg.popBuilder()

                cg.pushBuilder(block_cont)
                ov = cg.builder().load(new_ov)
                (ov, val) = _emitOne(ov, dep)
                return ov

            # Make sure we're attached.
            block_attached = cg.llvmNewBlock("attached")
            block_notattached = cg.llvmNewBlock("not-attached")

            cg.pushBuilder(block_notattached)
            cg.llvmRaiseExceptionByName("hlt_exception_overlay_not_attached", self.location())
            cg.popBuilder()

            _isEnd(cg, ov, 0, block_notattached, block_attached)

            cg.pushBuilder(block_attached) # Leave on stack.

            field = overlayt.field(self.op2().value().value())
            ov = _makeDep(ov, field)
            (ov, val) = _emitOne(ov, field)
            cg.llvmStoreInTarget(self, val)

    */
}
