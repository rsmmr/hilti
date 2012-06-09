
#include "declaration.h"
#include "statement.h"
#include "variable.h"
#include "function.h"
#include "instruction.h"
#include "builder/nodes.h"
#include "passes/printer.h"

using namespace hilti;

string Statement::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

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

string statement::Instruction::signature() const
{
    std::list<string> ops;

    auto op = _ops.begin();

    string target;

    if ( op != _ops.end() ) {
        if ( *op )
            target = util::fmt(" -> %s", (*op)->type()->render().c_str());

        while ( ++op != _ops.end() ) {
            if ( *op ) {
                auto o = (*op)->type()->render();
                ops.push_back(o);
            }
        }
    }

    return util::fmt("%s: %s%s", _id->name().c_str(), util::strjoin(ops, ", ").c_str(), target.c_str());
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

void statement::Block::addDeclarations(const decl_list& decls)
{
    for ( auto d : decls )
        addDeclaration(d);
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
    auto b = ast::as<statement::Block>(s);

    if ( b )
        return b->terminated();

    auto i = ast::as<statement::instruction::Resolved>(s);

    // fprintf(stderr, "XX %s %p %d\n", typeid(*(s.get())).name(), i.get(), (int)( i ? i->instruction()->terminator() : 0));

    if ( ! i )
        return false;

    return i->instruction()->terminator();
}

statement::instruction::Resolved::Resolved(shared_ptr<hilti::Instruction> instruction, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instruction->id(), ops, l) {
           _instruction = instruction;
       }

statement::instruction::Unresolved::Unresolved(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(id, ops, l) {}

statement::instruction::Unresolved::Unresolved(const ::string& name, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(shared_ptr<ID>(new ID(name)), ops, l) {}

statement::instruction::Unresolved::Unresolved(shared_ptr<hilti::Instruction> instr, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instr->id(), ops, l) { _instr = instr; }

statement::try_::Catch::Catch(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<Block> block, const Location& l)
    : Node(l)
{
    _id = id;
    _block = block;

    addChild(_id);
    addChild(_block);

    if ( id ) {
        auto var = std::make_shared<variable::Local>(id, type, nullptr, l);
        _decl = std::make_shared<declaration::Variable>(id, var, l);
        addChild(_decl);

        // Don't addChild type here, the variable will do it.
    }

    else {
        _type = type;
        addChild(_type);
    }
}

shared_ptr<variable::Local> statement::try_::Catch::Catch::variable() const
{
    return _decl ? ast::as<variable::Local>(_decl->variable()) : nullptr;
}

statement::Try::Try(shared_ptr<Block> try_, const catch_list& catches, const Location& l)
    : Statement(l)
{
    _try = try_;
    _catches = catches;

    addChild(try_);

    for ( auto c : _catches )
        addChild(c);
}

statement::ForEach::ForEach(shared_ptr<ID> id, shared_ptr<Expression> seq, shared_ptr<Block> body, const Location& l)
    : Statement(l)
{
    _id = id;
    _seq = seq;
    _body = body;

    addChild(_seq);
    addChild(_body);
}

shared_ptr<ID> statement::ForEach::ForEach::id() const
{
    return _id;
}
