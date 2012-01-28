
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::file::Close* i)
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
        cg.llvmCallC("hlt::file_close", [self.op1()])

*/
}

void StatementBuilder::visit(statement::instruction::file::Open* i)
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
        if self.op3():
            (ty, mode, charset) = self.op3().value().value()
        else:
            mod = cg.currentModule()
            ty = operand.ID(mod.scope().lookup("Hilti::FileType::Text"))
            mode = operand.ID(mod.scope().lookup("Hilti::FileMode::Create"))
            charset = operand.Constant(constant.Constant("utf8", type.String()))

        cg.llvmCallC("hlt::file_open", [self.op1(), self.op2(), ty, mode, charset])

*/
}

void StatementBuilder::visit(statement::instruction::file::Write* i)
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
        if isinstance(self.op2().type(), type.String):
            # String version.
            cg.llvmCallC("hlt::file_write_string", [self.op1(), self.op2()])
        else:
            # Bytes version.
            cg.llvmCallC("hlt::file_write_bytes", [self.op1(), self.op2()])

*/
}

