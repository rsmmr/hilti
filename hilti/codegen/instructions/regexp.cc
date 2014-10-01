
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

#include "libhilti/regexp.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::regexp::New* i)
{
    auto t = typedType(i->op1());

    int flags = 0;

    if ( t->attributes().has(attribute::NOSUB) )
        flags |= HLT_REGEXP_NOSUB;

    if ( t->attributes().has(attribute::FIRSTMATCH) )
        flags |= HLT_REGEXP_FIRST_MATCH;

    CodeGen::expr_list args = { builder::integer::create(flags) };
    auto result = cg()->llvmCall("hlt::regexp_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::CompileString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::regexp_compile", args);
}

void StatementBuilder::visit(statement::instruction::regexp::CompileSet* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::regexp_compile_set", args);
}


void StatementBuilder::visit(statement::instruction::regexp::FindString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::regexp_string_find", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::FindBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    if ( i->op3() )
        args.push_back(i->op3());
    else {
        auto end = builder::codegen::create(builder::iterator::typeBytes(), cg()->llvmIterBytesEnd());
        args.push_back(end);
    }

    auto result = cg()->llvmCall("hlt::regexp_bytes_find", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::GroupsString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::regexp_string_groups", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::GroupsBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    if ( i->op3() )
        args.push_back(i->op3());
    else {
        auto end = builder::codegen::create(builder::iterator::typeBytes(), cg()->llvmIterBytesEnd());
        args.push_back(end);
    }

    auto result = cg()->llvmCall("hlt::regexp_bytes_groups", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::regexp_string_match_token", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    if ( i->op3() )
        args.push_back(i->op3());
    else {
        auto end = builder::codegen::create(builder::iterator::typeBytes(), cg()->llvmIterBytesEnd());
        args.push_back(end);
    }

    auto result = cg()->llvmCall("hlt::regexp_bytes_match_token", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenAdvanceString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::regexp_string_match_token_advance", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::MatchTokenAdvanceBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    if ( i->op3() )
        args.push_back(i->op3());
    else {
        auto end = builder::codegen::create(builder::iterator::typeBytes(), cg()->llvmIterBytesEnd());
        args.push_back(end);
    }

    auto result = cg()->llvmCall("hlt::regexp_bytes_match_token_advance", args);

    cg()->llvmStore(i, result);
}


void StatementBuilder::visit(statement::instruction::regexp::MatchTokenInit* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::regexp_match_token_init", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::SpanString* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::regexp_string_span", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::regexp::SpanBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    if ( i->op3() )
        args.push_back(i->op3());
    else {
        auto end = builder::codegen::create(builder::iterator::typeBytes(), cg()->llvmIterBytesEnd());
        args.push_back(end);
    }

    auto result = cg()->llvmCall("hlt::regexp_bytes_span", args);

    cg()->llvmStore(i, result);
}
