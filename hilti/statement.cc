
#include "declaration.h"
#include "statement.h"
#include "variable.h"
#include "function.h"
#include "instruction.h"

using namespace hilti;

statement::Instruction::Instruction(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Statement(l)
{
    _id = id;
    addChild(_id);

    _ops = ops;
    for ( auto op : ops )
        addChild(op);
}

void statement::Block::addStatement(shared_ptr<Statement> stmt) {
       addComment(stmt);

       auto n = node_ptr<Statement>(stmt);
       _stmts.push_back(n);
       addChild(n);
   }

void statement::Block::addDeclaration(shared_ptr<Declaration> decl) {
       addComment(decl);

       auto n = node_ptr<Declaration>(decl);
       _decls.push_back(n);
       addChild(n);
   }

void statement::Block::Init(shared_ptr<Scope> parent, shared_ptr<ID> id, const decl_list& decls, const stmt_list& stmts, const Location& l)
{
    _id = id;
    _stmts = stmts;
    _decls = decls;
    _scope = unique_ptr<Scope>(new Scope(parent));

    if ( _id )
        addChild(_id);

    for ( auto i : stmts )
        addChild(i);

    for ( auto i : decls )
        addChild(i);
}

void statement::Block::addComment(shared_ptr<Node> node)
{
    if ( _next_comment == "" )
        return;

    node->setComment(_next_comment);
    _next_comment = "";
}

statement::instruction::Resolved::Resolved(shared_ptr<hilti::Instruction> instruction, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instruction->id(), ops) {
           _instruction = instruction;
       }

statement::instruction::Unresolved::Unresolved(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(id, ops) {}
