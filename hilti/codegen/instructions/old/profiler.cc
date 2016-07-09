
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::profiler::Start* i)
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
            if cg.profileLevel() == 0:
                return

            tag = (cg.llvmOp(self.op1()), self.op1().type())

            if self.op2():
                if isinstance(self.op2().type(), type.Enum):
                    style = self.op2()
                    param = (cg.llvmConstInt(0, 64), type.Integer(64))

                else:
                    style = self.op2().value().value()[0]
                    param = self.op2().value().value()[1]

                    if isinstance(param.type(), type.Interval):
                        param = param.type().llvmConstant(cg, param.value()), type.Integer(64)

            else:
                style =
       operand.ID(cg.currentModule().scope().lookup("Hilti::ProfileStyle::Standard"))
                param = (cg.llvmConstInt(0, 64), type.Integer(64))

            if self.op3():
                tmgr = self.op3()
            else:
                tmgr = (llvm.core.Constant.null(cg.llvmTypeGenericPointer()),
       type.Reference(type.TimerMgr()))

            cg.llvmCallC("hlt::profiler_start", [tag, style, param, tmgr])

    */
}

void StatementBuilder::visit(statement::instruction::profiler::Stop* i)
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
            if cg.profileLevel() == 0:
                return

            cg.llvmCallC("hlt::profiler_stop", [self.op1()])

    */
}

void StatementBuilder::visit(statement::instruction::profiler::Update* i)
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
            if cg.profileLevel() == 0:
                return

            if self.op2():
                op2 = (cg.llvmOp(self.op2()), self.op2().type())
            else:
                op2 = (cg.llvmConstInt(0, 64), type.Integer(64))

            cg.llvmCallC("hlt::profiler_update", [self.op1(), op2])

    */
}
