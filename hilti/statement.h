
#ifndef HILTI_STATEMENT_H
#define HILTI_STATEMENT_H

#include <ast/ast.h>

#include "common.h"
#include "id.h"
#include "instruction.h"
#include "scope.h"

namespace hilti {

/// Base class for AST statement nodes.
class Statement : public ast::Statement<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Statement(const Location& l=Location::None) : ast::Statement<AstInfo>(l) {}
   ACCEPT_VISITOR_ROOT();
};

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
       Init(parent, id, decls, stmts, l);
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
       Init(parent, id, decls, stmts, l);
   }

   /// Returns the block's name.
   shared_ptr<ID> id() const { return _id; }

   /// Returns the block's scope.
   shared_ptr<Scope> scope() const { return _scope; }

   /// Returns the block's statements.
   const stmt_list& Statements() const { return _stmts; }

   /// Returns the block's declaratations..
   const decl_list& Declarations() const { return _decls; }

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

   /// Adds a declaration to the block.
   ///
   /// stmt: The declarartion.
   void addDeclaration(shared_ptr<Declaration> decl);

   ACCEPT_VISITOR(Statement);

private:
   void Init(shared_ptr<Scope> parent, shared_ptr<ID> id, const decl_list& decls, const stmt_list& stmts, const Location& l=Location::None);
   void addComment(shared_ptr<Node> node);

   node_ptr<ID> _id;
   shared_ptr<Scope> _scope;
   string _next_comment = "";
   decl_list _decls;
   stmt_list _stmts;
   block_list _subblocks;
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

   void render(std::ostream& out) const /* override */ { out << _id->name(); }

   ACCEPT_VISITOR(Statement);

protected:
   /// Constructor.
   ///
   /// id: The name of the instruction.
   ///
   /// ops: The list of operands. The first is the target and may be null if none is used.
   ///
   /// l: An associated location.
   Instruction(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l=Location::None);

private:
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

   ACCEPT_VISITOR(Instruction);
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

   ACCEPT_VISITOR(Instruction);

private:
   shared_ptr<hilti::Instruction> _instruction;
};

}

}

}

#endif
