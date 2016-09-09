
#ifndef BINPAC_STATEMENT_H
#define BINPAC_STATEMENT_H

#include <ast/statement.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

/// Base class for AST statement nodes.
class Statement : public ast::Statement<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: An associated location.
    Statement(const Location& l = Location::None);

    string render() override;

    ACCEPT_VISITOR_ROOT();
};

namespace statement {

/// A statement block.
class Block : public Statement {
    AST_RTTI
public:
    typedef std::list<node_ptr<statement::Block>> block_list;

    /// Constructor.
    ///
    /// parent: A parent scope. It will be used to resolve unknown ID
    /// references. Can be null if no parent.
    ///
    /// l: An associated location.
    Block(shared_ptr<Scope> parent, const Location& l = Location::None);

    /// Returns the block's scope.
    shared_ptr<Scope> scope() const;

    /// Returns the block's statements.
    statement_list statements() const;

    /// Returns the block's declaratations..
    declaration_list declarations() const;

    /// Associates a comment with the \a next statement that will be added to
    /// the block via addStatement().
    ///
    /// comment: The comment.
    void setNextComment(string comment);

    /// Adds a statement to the block.
    ///
    /// stmt: The statement.
    void addStatement(shared_ptr<Statement> stmt);

    /// Adds a statement to the front of the block.
    ///
    /// stmt: The statement.
    void addStatementAtFront(shared_ptr<Statement> stmt);

    /// Adds a series of statements to the block.
    ///
    /// stmt: The statement.
    void addStatements(const statement_list& stmts);

    /// Adds a declaration to the block.
    ///
    /// stmt: The declarartion.
    void addDeclaration(shared_ptr<Declaration> decl);

    /// Adds a series of declarations to the block.
    ///
    /// decls; The declarations.
    void addDeclarations(const declaration_list& decls);

    ACCEPT_VISITOR(Statement);

private:
    void addComment(shared_ptr<Node> node);

    shared_ptr<Scope> _scope;
    string _next_comment = "";
    std::list<node_ptr<Declaration>> _decls;
    std::list<node_ptr<Statement>> _stmts;
};

namespace try_ {

/// A single catch clause for a Try statement.
class Catch : public Node {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// type: The type being caught. This must be of type type::Exception.
    ///
    /// id: The local ID assigned to the caught exception instance. It will be
    /// assible in \a block.
    ///
    /// block: The code block for the catch.
    Catch(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<Block> block,
          const Location& l = Location::None);

    /// Constructor for a "catch all".
    ///
    /// block: The code block for the catch.
    Catch(shared_ptr<Block> block, const Location& l = Location::None);

    /// Returns the type being caught. This will be of type type::Exception.
    /// Will return null for a catch-all clause.
    shared_ptr<Type> type() const;

    /// Returns the local ID assigned to the caught exception instance. Will
    /// return null for a catch-all clause.
    shared_ptr<ID> id() const;

    /// Returns the catch's code block.
    shared_ptr<Block> block() const;

    /// If an ID was given to the constructor, this returns a local variable
    /// referecning the exception value. If not, it returns null.
    shared_ptr<variable::Local> variable() const;

    /// Returns true if this is a catch-all clause.
    bool catchAll() const;

    ACCEPT_VISITOR_ROOT();

private:
    node_ptr<Type> _type = nullptr;
    node_ptr<ID> _id = nullptr;
    node_ptr<Block> _block;
    node_ptr<declaration::Variable> _decl = nullptr;
};
}

/// A try-encapsulated block, with \a catch handlers.
class Try : public Statement {
    AST_RTTI
public:
    typedef std::list<node_ptr<try_::Catch>> catch_list;

    /// Constructor.
    ///
    /// try_: The body of the \c try block. Exceptions occuring during its
    /// execution will matches against all the \a catches.
    ///
    /// catches: List of \c catch clauses handling exceptions thrown during
    /// executing \a try. Note that there must not be more than one catch-all
    /// clause.
    Try(shared_ptr<Block> try_, const catch_list& catches, const Location& l = Location::None);

    /// Returns the \a try block.
    shared_ptr<Block> block() const;

    /// Returns the list of all catch clauses.
    const catch_list& catches() const;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<Block> _try;
    catch_list _catches;
};

/// A for-each loop iterating over a sequence.
class ForEach : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The iteration variable.
    ///
    /// seq: The sequence to iterate over. The type must be of trait trait::Iterable.
    ///
    /// body: The loop body.
    ///
    /// l: Associated location information.
    ForEach(shared_ptr<ID> id, shared_ptr<binpac::Expression> seq, shared_ptr<Block> body,
            const Location& l = Location::None);

    /// Returns the iteration variable.
    shared_ptr<ID> id() const;

    /// Returns the seq block.
    shared_ptr<binpac::Expression> sequence() const;

    /// Returns the body block.
    shared_ptr<Block> body() const;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<binpac::Expression> _seq;
    node_ptr<Block> _body;

    shared_ptr<ID> _id; // No node_ptr, we just want to remember this here.
};

/// A no-op statement.
class NoOp : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location information.
    NoOp(const Location& l = Location::None);

    ACCEPT_VISITOR(Statement);
};

/// A ``print`` statement.
class Print : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// exprs: list of ~Expression - The arguments to print.
    ///
    /// l: Associated location.
    Print(const expression_list& exprs, const Location& l = Location::None);

    expression_list expressions() const;

    ACCEPT_VISITOR(Statement);

private:
    std::list<node_ptr<binpac::Expression>> _exprs;
};


/// A statement evaluating an expression.
class Expression : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// expr: The expression.
    ///
    /// l: Associated location.
    Expression(shared_ptr<binpac::Expression> expr, const Location& l = Location::None);

    shared_ptr<binpac::Expression> expression() const;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<binpac::Expression> _expr;
};


/// A statement branching to one of two alternatives based on the value of an
/// an expression.
class IfElse : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// cond: The expression to evaluate; must be coercable to a boolean
    /// value.
    ///
    /// true_: The statement to execute when *cond* is false.
    ///
    /// false: The statement to execute when *cond* is true. Can be null if in
    /// this case, nothing should be done.
    ///
    ///    /// Constructor.
    /// l: Associated location information.
    IfElse(shared_ptr<binpac::Expression> cond, shared_ptr<Statement> true_,
           shared_ptr<Statement> false_ = nullptr, const Location& l = Location::None);

    /// Returns the condition.
    shared_ptr<binpac::Expression> condition() const;

    /// Returns the true branch.
    shared_ptr<Statement> statementTrue() const;

    /// Returns the false branch. This may be null if not given.
    shared_ptr<Statement> statementFalse() const;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<binpac::Expression> _cond;
    node_ptr<Statement> _true;
    node_ptr<Statement> _false;
};

/// A return statement.
class Return : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// result: The result to return, or null for none.
    ///
    /// l: Associated location information.
    Return(shared_ptr<binpac::Expression> expr, const Location& l = Location::None);

    /// Returns the expression to return, or null if none.
    shared_ptr<binpac::Expression> expression() const;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<binpac::Expression> _expr;
};

/// A hook "stop" statement, as used in \c foreach hooks.
class Stop : public Statement {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location information.
    Stop(const Location& l = Location::None);

    ACCEPT_VISITOR(Statement);
};
}
}

#endif
