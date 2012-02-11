
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

    while ( _ops.size() < 4 )
        _ops.push_back(shared_ptr<Expression>());
}

void statement::Block::addStatement(shared_ptr<Statement> stmt)
{
       addComment(stmt);

       auto n = node_ptr<Statement>(stmt);
       _stmts.push_back(n);
       addChild(n);
}

void statement::Block::addStatementAtFront(shared_ptr<Statement> stmt)
{
       addComment(stmt);

       auto n = node_ptr<Statement>(stmt);
       _stmts.push_front(n);
       addChild(n);
}

void statement::Block::addStatements(const stmt_list& stmts)
{
    for ( auto s : stmts )
        addStatement(s);
}

void statement::Block::addDeclaration(shared_ptr<Declaration> decl)
{
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

bool statement::Block::terminated() const
{
    if ( ! _stmts.size() )
        return false;

    shared_ptr<Statement> s = _stmts.back();

    auto i = ast::as<statement::instruction::Resolved>(s);

    // fprintf(stderr, "XX %s %p %d\n", typeid(*(s.get())).name(), i.get(), (int)( i ? i->instruction()->terminator() : 0));

    if ( ! i )
        return false;

    return i->instruction()->terminator();
}

statement::instruction::Resolved::Resolved(shared_ptr<hilti::Instruction> instruction, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instruction->id(), ops) {
           _instruction = instruction;
       }

statement::instruction::Unresolved::Unresolved(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(id, ops) {}

statement::instruction::Unresolved::Unresolved(const ::string& name, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(shared_ptr<ID>(new ID(name)), ops) {}
