
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::classifier::Add* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
        auto op3 = cg()->llvmValue(i->op3(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        args.push_back(i->op3());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            rtype = self.op1().type().refType().ruleType()

            if isinstance(self.op2().type(), type.Tuple) and \
               isinstance(self.op2().type().types()[0], type.Reference) and \
               self.op2().type().types()[0].refType() == rtype:

                op2 = cg.llvmOp(self.op2())
                rule = self.op2().type().llvmIndex(cg, op2, 0)
                prio = self.op2().type().llvmIndex(cg, op2, 1)
                vtype = self.op2().type().types()[0].refType()

            else:
                vtype = rtype
                rule = self.op2().coerceTo(cg, type.Reference(rtype))
                rule = cg.llvmOp(rule)
                prio = None

            fields = _llvmFields(cg, rtype, vtype, rule)

            if prio != None:
                cg.llvmCallC("hlt::classifier_add", [self.op1(), fields, operand.LLVM(prio,
       self.op2().type().types()[1]), self.op3()])
            else:
                cg.llvmCallC("hlt::classifier_add_no_prio", [self.op1(), fields, self.op3()])

    */
}

void StatementBuilder::visit(statement::instruction::classifier::Compile* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            cg.llvmCallC("hlt::classifier_compile", [self.op1()])

    */
}

void StatementBuilder::visit(statement::instruction::classifier::Get* i)
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
            op2 = self.op2()
            rtype = self.op1().type().refType().ruleType()
            vtype = self.op2().type()

            if isinstance(vtype, type.Tuple):
                # Turn the tuple into a struct.
                op2.coerceTo(cg, type.Reference(rtype))

            fields = _llvmFields(cg, rtype, rtype, cg.llvmOp(op2))

            voidp = cg.llvmCallC("hlt::classifier_get", [self.op1(), fields])
            casted = cg.builder().bitcast(voidp,
       llvm.core.Type.pointer(cg.llvmType(self.target().type())))
            cg.llvmStoreInTarget(self, cg.builder().load(casted))

    */
}

void StatementBuilder::visit(statement::instruction::classifier::Matches* i)
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
            op2 = self.op2()
            rtype = self.op1().type().refType().ruleType()
            vtype = self.op2().type()

            if isinstance(vtype, type.Tuple):
                # Turn the tuple into a struct.
                op2.coerceTo(cg, type.Reference(rtype))

            fields = _llvmFields(cg, rtype, rtype, cg.llvmOp(op2))

            result = cg.llvmCallC("hlt::classifier_matches", [self.op1(), fields])
            cg.llvmStoreInTarget(self, result)

    */
}
