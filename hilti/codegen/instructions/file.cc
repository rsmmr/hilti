
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::file::New* i)
{
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::file_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::file::Close* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::file_close", args);
}

void StatementBuilder::visit(statement::instruction::file::Open* i)
{
    shared_ptr<Expression> op3 = i->op3();

    llvm::Value* ty = nullptr;
    llvm::Value* mode = nullptr;
    llvm::Value* chr = nullptr;

    if ( i->op3() ) {
        auto llvm_op3 = cg()->llvmValue(i->op3());
        ty = cg()->llvmTupleElement(i->op3()->type(), llvm_op3, 0, false);
        mode = cg()->llvmTupleElement(i->op3()->type(), llvm_op3, 1, false);
        chr = cg()->llvmTupleElement(i->op3()->type(), llvm_op3, 2, false);
    }

    else {
        // Use defaults.
        ty = cg()->llvmEnum("Hilti::FileType::Text");
        mode = cg()->llvmEnum("Hilti::FileMode::Create");
        chr = cg()->llvmString("utf8");
    }

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(builder::codegen::create(cg()->typeByName("Hilti::FileType"), ty, i->location()));
    args.push_back(builder::codegen::create(cg()->typeByName("Hilti::FileMode"), mode, i->location()));
    args.push_back(builder::codegen::create(builder::string::type(), chr, i->location()));
    cg()->llvmCall("hlt::file_open", args);

}

void StatementBuilder::visit(statement::instruction::file::WriteBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::file_write_bytes", args);
}

void StatementBuilder::visit(statement::instruction::file::WriteString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::file_write_string", args);
}
