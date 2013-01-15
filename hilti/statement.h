
#ifndef HILTI_STATEMENT_H
# define HILTI_STATEMENT_H

# include <ast/statement.h>

# include "common.h"
# include "id.h"
# include "instruction.h"
# include "scope.h"
# include "declaration.h"

namespace hilti {
namespace passes {
    class InstructionResolver;
}

/// Base class for AST statement nodes.
class Statement : public ast::Statement<AstInfo>
{
public:
    /// Constructor.
    ///
    /// l: An associated location.
    Statement(const Location& l=Location::None);

    /// Structure to describe a variable for data flow analysis.
    ///
    /// \todo: Right now we model only local variables and parameters and
    /// hence the names need to be unique only within functions. If we add
    /// globals, we need to change that. 
    struct FlowVariable {
        FlowVariable(shared_ptr<expression::Variable> variable);
        FlowVariable(shared_ptr<expression::Parameter> param);

        FlowVariable& operator=(const FlowVariable& other);
        bool operator==(const FlowVariable& other) const;
        bool operator<(const FlowVariable& other) const;

        string name;                       /// Name for this variable unique within its function. Used for comparision.
        shared_ptr<Expression> expression; /// Expression to reference the variable.
    };

    struct FlowVariablePtrCmp {
        bool operator()(const shared_ptr<Statement::FlowVariable>& f1, const shared_ptr<Statement::FlowVariable>& f2);
    };

    typedef std::set<shared_ptr<Statement::FlowVariable>, FlowVariablePtrCmp> variable_set;

    /// Provides data and control flow information for the statement.
    struct FlowInfo {
        /// Variables defined by this instruction, i.e., overwritten with a
        /// new valid value.
        variable_set defined;

        /// Variables cleared by this instruction, i.e., redefined to no
        /// longer have a valid value.
        variable_set cleared;

        /// Variables that have their current value potentially modified by
        /// this instruction, but not reset/cleared to a new independent value/
        variable_set modified;

        /// Variables potentially read by this instructionm but not modified.
        /// This is a superset of \a defined and \a cleared.
        variable_set read;

        // All potential successor blocks as determined by any of the
        // statements operands.
        std::set<shared_ptr<statement::Block>> successors;
    };

    /// Returns a number identying the statement which is unique across all statements.
    uint64_t number() const { return _number; }

    /// Returns the direct successor statement if the statement is part of a
    /// block. Note that flowInfo() instead gives access to successor
    /// *blocks*.
    shared_ptr<Statement> successor() const;

    /// Returns the statement's flow information.
    ///
    /// Must be overridden by derived classed. This implementation should add
    /// all relevant declarations, the parent \a FlowInfo() call will
    /// filter them accordingly to what's requested.
    virtual FlowInfo flowInfo() = 0;

    /// Returns the first statement that's not a block, according to control
    /// flow. Returns null if there's none.
    ///
    /// The default implementation returns the statement itself and is fine
    /// for atomic statements (and potentially for others as well).
    virtual shared_ptr<Statement> firstNonBlock();

    /// Returns a readable one-line representation of the type.
    string render() override;

    ACCEPT_VISITOR_ROOT();

protected:
    friend class passes::InstructionResolver;

    /// Sets the successor instruction. Normally used just by the instruction
    /// resolver.
    void setSuccessor(shared_ptr<Statement> stmt);

private:
    shared_ptr<Statement> _successor = 0; // Not a node ptr, we don't add it as a child.
    uint64_t _number;

    static uint64_t _counter;
};

extern bool operator==(const shared_ptr<Statement::FlowVariable>& v1, const shared_ptr<Statement::FlowVariable>& v2);

namespace statement {

/// A no-op statement.
///
/// \todo This should move into \a ast.
class Noop : public Statement
{
public:
    /// Constructor.
    ///
    /// l: An associated location.
    Noop(const Location& l=Location::None) : Statement(l) {}

    FlowInfo flowInfo() override { return FlowInfo(); }

    ACCEPT_VISITOR(Statement);

};

/// A block statement. A block has associated with it (1) an optional name to
/// reference it; (2) a set of declarations visible in the block's scope; and
/// (3) and set of statements making up the code body.
class Block : public Statement
{
public:
    typedef std::list<node_ptr<Statement>> stmt_list;
    typedef std::list<node_ptr<Declaration>> decl_list;
    typedef std::list<node_ptr<statement::Block>> block_list;

    /// Constructor.
    ///
    /// parent: A parent scope. It will be used to resolve unknown ID
    /// references. Can be null if no parent.
    ///
    /// id: An optional name for the block.
    ///
    ///
    /// l: An associated location.
    Block(shared_ptr<Scope> parent, shared_ptr<ID> id = nullptr, const Location& l=Location::None) : Statement(l) {
        decl_list decls;
        stmt_list stmts;
        _init(parent, id, decls, stmts, l);
    }

    /// Constructor.
    ///
    /// decls: A list of declarations for the block's scope.
    ///
    /// stmts: A list of statements for the block's code body.
    ///
    /// parent: A parent scope. It will be used to resolve unknown ID
    /// references. Can be null if no parent.
    ///
    /// id: An optional name for the block.
    ///
    /// l: An associated location.
    Block(const decl_list& decls, const stmt_list& stmts, shared_ptr<Scope> parent, shared_ptr<ID> id = nullptr, const Location& l=Location::None) : Statement(l) {
        _init(parent, id, decls, stmts, l);
    }

    /// Returns the block's name.
    shared_ptr<ID> id() const { return _id; }

    /// Sets the block's name.
    ///
    /// id: The name.
    void setID(shared_ptr<ID> id) { _id = id;}

    /// Returns the block's scope.
    shared_ptr<Scope> scope() const { return _scope; }

    /// Sets the block's scope.
    void setScope(shared_ptr<Scope> scope) { _scope = scope; }

    /// Returns the block's statements.
    const stmt_list& statements() const { return _stmts; }

    /// Returns the block's declaratations..
    const decl_list& declarations() const { return _decls; }

    /// Associates a comment with the \a next statement that will be added to
    /// the block via addStatement().
    ///
    /// comment: The comment.
    void setNextComment(string comment) {
        _next_comment = comment;
    }

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
    void addStatements(const stmt_list& stmts);

    /// Adds a declaration to the block.
    ///
    /// stmt: The declarartion.
    void addDeclaration(shared_ptr<Declaration> decl);

    /// Adds a series of declarations to the block.
    ///
    /// decls; The declarations.
    void addDeclarations(const decl_list& decls);

    /// Returns true if the last statement of the block is a terminator
    /// instruction.
    bool terminated() const;

    /// Remove statements after a terminator.
    void removeUseless();

    FlowInfo flowInfo() override;

    shared_ptr<Statement> firstNonBlock() override;

    ACCEPT_VISITOR(Statement);

private:
    void _init(shared_ptr<Scope> parent, shared_ptr<ID> id, const decl_list& decls, const stmt_list& stmts, const Location& l=Location::None);
    void _addComment(shared_ptr<Node> node);
    shared_ptr<Statement> _firstNonBlock(shared_ptr<statement::Block> block, std::set<shared_ptr<statement::Block>>* done);

    node_ptr<ID> _id;
    shared_ptr<Scope> _scope;
    string _next_comment = "";
    decl_list _decls;
    stmt_list _stmts;
    block_list _subblocks;
};

namespace try_ {

/// A single catch clause for a Try statement.
class Catch : public Node
{
public:
    /// Constructor.
    ///
    /// type: The type being caught. This must be of type type::Exception. Can
    /// be null for "catch all".
    ///
    /// id: The local ID assigned to the caught exception instance. It will be
    /// assible in \a block. Must be null if \a type is.
    ///
    /// block: The code block for the catch.
    Catch(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<Block> block, const Location& l=Location::None);

    /// Returns the type being caught. This will be of type type::Exception.
    /// Will return null for a catch-all clause.
    shared_ptr<Type> type() const { if ( _type ) return _type; else return variable() ? variable()->type() : nullptr; }

    /// Returns the local ID assigned to the caught exception instance. Will
    /// return null for a catch-all clause.
    shared_ptr<ID> id() const { return _id; }

    /// Returns the catch's code block.
    shared_ptr<Block> block() const { return _block; }

    /// If an ID was given to the constructor, this returns a local variable
    /// referecning the exception value. If not, it returns null.
    shared_ptr<variable::Local> variable() const;

    /// Returns true if this is a catch-all clause.
    bool catchAll() const { return _type.get() == nullptr && _id.get() == nullptr; }

    ACCEPT_VISITOR_ROOT();

private:
    node_ptr<Type> _type = nullptr;
    node_ptr<ID> _id = nullptr;
    node_ptr<Block> _block;
    node_ptr<declaration::Variable> _decl = nullptr;
};

}

/// A try-encapsulated block, with \a catch handlers.
class Try : public Statement
{
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
    Try(shared_ptr<Block> try_, const catch_list& catches, const Location& l=Location::None);

    /// Returns the \a try block.
    shared_ptr<Block> block() const { return _try; }

    /// Returns the list of all catch clauses.
    const catch_list& catches() const { return _catches; }

    FlowInfo flowInfo() override;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<Block> _try;
    catch_list _catches;
};

/// A for-each loop iterating over a sequence.
class ForEach : public Statement
{
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
    ForEach(shared_ptr<ID> id, shared_ptr<Expression> seq, shared_ptr<Block> body, const Location& l=Location::None);

    /// Returns the iteration variable.
    shared_ptr<ID> id() const;

    /// Returns the seq block.
    shared_ptr<Expression> sequence() const { return _seq; }

    /// Returns the body block.
    shared_ptr<Block> body() const { return _body; }

    FlowInfo flowInfo() override;

    ACCEPT_VISITOR(Statement);

private:
    node_ptr<Expression> _seq;
    node_ptr<Block> _body;

    shared_ptr<ID> _id; // No node_ptr, we just want to remember this here.
};

/// Base class for instruction statements.
class Instruction : public Statement
{
public:
    /// Returns the name of the instruction.
    const shared_ptr<ID> id() const { return _id; }

    /// Returns the instructions operands. The first is the target and will be
    /// null if not used.
    const hilti::instruction::Operands& operands() const { return _ops; }

    /// Returns the target operand. Null if not used.
    shared_ptr<Expression> target() const { return _ops[0]; }

    /// Returns the 1st operand. Null if not used.
    shared_ptr<Expression> op1() const { return _ops[1]; }

    /// Returns the 2st operand. Null if not used.
    shared_ptr<Expression> op2() const { return _ops[2]; }

    /// Returns the 3st operand. Null if not used.
    shared_ptr<Expression> op3() const { return _ops[3]; }

    /// Returns a text representation of the instructions operand signature.
    string signature() const;

    /// Returns true if this function has been marked as internally added.
    bool internal() const { return _internal; }

    /// Marks this function as internally added.
    void setInternal() { _internal = true; }

    ACCEPT_VISITOR(Statement);

    /// Constructor.
    ///
    /// id: The name of the instruction.
    ///
    /// ops: The list of operands. The first is the target and may be null if
    /// none is used. If the the list has less than 4 elements, it will be
    /// filled with nullptr corresponding to unused operands.
    ///
    /// l: An associated location.
    Instruction(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l=Location::None);

private:
    bool _internal = false;
    node_ptr<ID> _id;
    hilti::instruction::Operands _ops;
};

namespace instruction {

/// A not yet resolved instruction. Initially, instructions are instantiated
/// as Unresolved and later turned into instances derived from Resolved by
/// passes::InstructionResolver.
class Unresolved : public Instruction
{
public:
    /// Constructor.
    ///
    /// id: The name of the instruction.
    ///
    /// ops: The list of operands. The first is the target and may be null if none is used.
    ///
    /// l: An associated location.
    Unresolved(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l=Location::None);

    /// Constructor.
    ///
    /// name: The name of the instruction.
    ///
    /// ops: The list of operands. The first is the target and may be null if none is used.
    ///
    /// l: An associated location.
    Unresolved(const ::string& name, const hilti::instruction::Operands& ops, const Location& l=Location::None);

    /// Constructor. This variant is for when the instruction itself is
    /// already known, but not all operands may be resolved yet.
    ///
    /// instr: The instruction.
    ///
    /// ops: The list of operands. The first is the target and may be null if none is used.
    ///
    /// l: An associated location.
    Unresolved(shared_ptr<hilti::Instruction> instr, const hilti::instruction::Operands& ops, const Location& l=Location::None);

    /// If an instruction instance was passed to the constructor, this
    /// returned; otherwise null.
    shared_ptr<hilti::Instruction> instruction() const { return _instr; }

    FlowInfo flowInfo() override;

    ACCEPT_VISITOR(Instruction);

private:
    shared_ptr<hilti::Instruction> _instr;
};


/// A resolved instruction. For each type of instructions, there is one class
/// derived from this base (the macro magic in
/// instructions/define-instruction.h does that). For each Unresolved
/// instruction, the passes::InstructionResolver instantiates the right
/// class.
class Resolved : public Instruction
{
public:
    /// Constructor.
    ///
    /// instruction: The instruction the statement uses.
    ///
    /// ops: The list of operands. The first is the target and may be null if none is used.
    ///
    /// l: An associated location.
    Resolved(shared_ptr<hilti::Instruction> instruction, const hilti::instruction::Operands& ops, const Location& l=Location::None);

    /// Returns the instruction the statement uses.
    const shared_ptr<hilti::Instruction> instruction() const { return _instruction; }

    FlowInfo flowInfo() override;

    ACCEPT_VISITOR(Instruction);

private:
    shared_ptr<hilti::Instruction> _instruction;
};

}

}

}

#endif
