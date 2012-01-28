
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::regexp::Compile* i)
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
            cg.llvmCallC("hlt::regexp_compile", [self.op1(), self.op2()])
        else:
            cg.llvmCallC("hlt::regexp_compile_set", [self.op1(), self.op2()])

*/
}

void StatementBuilder::visit(statement::instruction::regexp::Find* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_find", [self.op1(), self.op2()])
        else:
            op3 = self.op3() if self.op3() else operand.LLVM(bytes.llvmEnd(cg), type.IteratorBytes())
            result = cg.llvmCallC("hlt::regexp_bytes_find", [self.op1(), self.op2(), op3])

        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::regexp::Groups* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_groups", [self.op1(), self.op2()])
        else:
            # Bytes version.
            result = cg.llvmCallC("hlt::regexp_bytes_groups", [self.op1(), self.op2(), self.op3()])

        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::regexp::MatchToken* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_match_token", [self.op1(), self.op2()])
        else:
            # Bytes version.
            op3 = self.op3() if self.op3() else operand.LLVM(bytes.llvmEnd(cg), type.IteratorBytes())
            result = cg.llvmCallC("hlt::regexp_bytes_match_token", [self.op1(), self.op2(), op3])

        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenInit* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            assert False # Not implmented.
        else:
            # Bytes version.
            op3 = self.op3() if self.op3() else operand.LLVM(bytes.llvmEnd(cg), type.IteratorBytes())
            result = cg.llvmCallC("hlt::regexp_bytes_match_token_advance", [self.op1(), self.op2(), op3])

        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenInit* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::regexp_match_token_init", [self.op1()])
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::regexp::Span* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_span", [self.op1(), self.op2()])
        else:
            # Bytes version.
            result = cg.llvmCallC("hlt::regexp_bytes_span", [self.op1(), self.op2(), self.op3()])

        cg.llvmStoreInTarget(self, result)

*/
}

