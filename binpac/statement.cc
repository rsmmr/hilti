
#include "statement.h"
#include "declaration.h"
#include "expression.h"
#include "scope.h"
#include "type.h"
#include "variable.h"

#include "passes/printer.h"

using namespace binpac;
using namespace binpac::statement;

string Statement::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

Statement::Statement(const Location& l) : ast::Statement<AstInfo>(l)
{
}

Block::Block(shared_ptr<Scope> parent, const Location& l) : Statement(l)
{
    _scope = unique_ptr<Scope>(new Scope(parent));
}

void Block::addStatement(shared_ptr<Statement> stmt)
{
    addComment(stmt);

    _stmts.push_back(stmt);
    addChild(_stmts.back());
}

void Block::addStatementAtFront(shared_ptr<Statement> stmt)
{
    addComment(stmt);

    _stmts.push_front(stmt);
    addChild(_stmts.front());
}

void Block::addStatements(const statement_list& stmts)
{
    for ( auto s : stmts )
        addStatement(s);
}

void Block::addDeclaration(shared_ptr<Declaration> decl)
{
    addComment(decl);

    _decls.push_back(decl);
    addChild(_decls.back());
}

void Block::addDeclarations(const declaration_list& decls)
{
    for ( auto d : decls )
        addDeclaration(d);
}

statement_list Block::statements() const
{
    statement_list stmts;
    for ( auto s : _stmts )
        stmts.push_back(s);

    return stmts;
}

declaration_list Block::declarations() const
{
    declaration_list decls;
    for ( auto s : _decls )
        decls.push_back(s);

    return decls;
}


void Block::addComment(shared_ptr<Node> node)
{
    if ( _next_comment == "" )
        return;

    node->addComment(_next_comment);
    _next_comment = "";
}

void Block::setNextComment(string comment)
{
    _next_comment = comment;
}

shared_ptr<Scope> Block::scope() const
{
    return _scope;
}

try_::Catch::Catch(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<Block> block,
                   const Location& l)
    : Node(l)
{
    _id = id;
    _block = block;

    addChild(_id);
    addChild(_block);

    if ( id ) {
        auto var = std::make_shared<variable::Local>(id, type, nullptr, l);
        _decl = std::make_shared<declaration::Variable>(id, Declaration::LOCAL, var, l);
        addChild(_decl);

        // Don't addChild type here, the variable will do it.
    }

    else {
        _type = type;
        addChild(_type);
    }
}

try_::Catch::Catch(shared_ptr<Block> block, const Location& l) : Node(l)
{
    _block = block;
    addChild(_block);
}

shared_ptr<variable::Local> try_::Catch::Catch::variable() const
{
    return _decl ? ast::checkedCast<variable::Local>(_decl->variable()) : nullptr;
}

shared_ptr<Type> try_::Catch::type() const
{
    if ( _type )
        return _type;

    else
        return variable() ? variable()->type() : nullptr;
}

bool try_::Catch::catchAll() const
{
    return _type.get() == nullptr && _id.get() == nullptr;
}

shared_ptr<ID> try_::Catch::id() const
{
    return _id;
}

shared_ptr<Block> try_::Catch::block() const
{
    return _block;
}

Try::Try(shared_ptr<Block> try_, const catch_list& catches, const Location& l) : Statement(l)
{
    _try = try_;
    _catches = catches;

    addChild(try_);

    for ( auto c : _catches )
        addChild(c);
}

const Try::catch_list& Try::catches() const
{
    return _catches;
}

shared_ptr<Block> Try::block() const
{
    return _try;
}


ForEach::ForEach(shared_ptr<ID> id, shared_ptr<binpac::Expression> seq, shared_ptr<Block> body,
                 const Location& l)
    : Statement(l)
{
    _id = id;
    _seq = seq;
    _body = body;

    addChild(_seq);
    addChild(_body);
}

shared_ptr<ID> ForEach::ForEach::id() const
{
    return _id;
}

shared_ptr<binpac::Expression> ForEach::sequence() const
{
    return _seq;
}

shared_ptr<Block> ForEach::body() const
{
    return _body;
}

NoOp::NoOp(const Location& l) : Statement(l)
{
}

Print::Print(const expression_list& exprs, const Location& l) : Statement(l)
{
    for ( auto e : exprs )
        _exprs.push_back(e);

    for ( auto e : _exprs )
        addChild(e);
}

expression_list Print::expressions() const
{
    expression_list exprs;

    for ( auto e : _exprs )
        exprs.push_back(e);

    return exprs;
}

statement::Expression::Expression(shared_ptr<binpac::Expression> expr, const Location& l)
    : Statement(l)
{
    _expr = expr;
    addChild(_expr);
}

shared_ptr<binpac::Expression> statement::Expression::expression() const
{
    return _expr;
}

IfElse::IfElse(shared_ptr<binpac::Expression> cond, shared_ptr<Statement> true_,
               shared_ptr<Statement> false_, const Location& l)
    : Statement(l)
{
    _cond = cond;
    _true = true_;
    _false = false_;

    addChild(_cond);
    addChild(_true);
    addChild(_false);
}

shared_ptr<binpac::Expression> IfElse::condition() const
{
    return _cond;
}

shared_ptr<Statement> IfElse::statementTrue() const
{
    return _true;
}

shared_ptr<Statement> IfElse::statementFalse() const
{
    return _false;
}

Return::Return(shared_ptr<binpac::Expression> expr, const Location& l) : Statement(l)
{
    _expr = expr;
    addChild(_expr);
}

shared_ptr<binpac::Expression> Return::expression() const
{
    return _expr;
}

Stop::Stop(const Location& l)
{
}
