
#ifndef HILTI_BUILDER_BLOCK_H
#define HILTI_BUILDER_BLOCK_H

#include "hilti-intern.h"

namespace hilti {
namespace builder {

class ModuleBuilder;

/// Class for building blocks. It comes with a number of helper methods for
/// building some constructs more easily than by using the static \a
/// builder::* functions, but using them is optionally.
class BlockBuilder
{
public:
   /// Destructor.
   ~BlockBuilder();

   /// Returns an expression referencing the block. The expression can
   /// directly be used with instructions taking a label argument, just as
   /// branch instructions.
   shared_ptr<hilti::expression::Block> block() const;

   /// Returns the ModuleBuilder() that this builder is associated with.
   ModuleBuilder* moduleBuilder() const;

   /// Adds a local variable to the block's scope.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the local with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists in
   /// the function's scope, adapts the name to be unique. If false and the
   /// name is already taken, aborts with internal error if the declarations
   /// aren't equivalent; if they are, returns the existing one.
   ///
   /// Returns: An expression referencing the local.
   shared_ptr<hilti::expression::Variable> addLocal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<hilti::Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a local variable to the block's scope.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the local with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists in
   /// the function's scope, adapts the name to be unique. If false and the
   /// name is already taken, aborts with internal error if the declarations
   /// aren't equivalent; if they are, returns the existing one.
   ///
   /// Returns: An expression referencing the local.
   shared_ptr<hilti::expression::Variable> addLocal(const std::string& id, shared_ptr<Type> type, shared_ptr<hilti::Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a temporary variable to the current function. This is a
   /// convininece method that just forwards to the corresponding
   /// ModuleBuilder::addTmp().
   ///
   /// id: An ID that will be used to build the name of the temporary
   /// variable. The methods always creates a new unique temporary and adapts
   /// the name as necessary (except for \a reuse, as below).
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// reuse: If true and a temporary of that name already exists, return
   /// that one. The types must be equivalent, if they don't, that's an
   /// internal error.
   ///
   /// Returns: An expression referencing the variable.
   ///
   shared_ptr<hilti::expression::Variable> addTmp(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<hilti::Expression> init = nullptr, bool reuse = false, const Location& l = Location::None);

   /// Adds a temporary variable to the current function. This is a
   /// convininece method that just forwards to the corresponding
   /// ModuleBuilder::addTmp().
   ///
   /// id: An ID that will be used to build the name of the temporary
   /// variable. The methods always creates a new unique temporary and adapts
   /// the name as necessary (except for \a reuse, as below).
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// reuse: If true and a temporary of that name already exists, return
   /// that one. The types must be equivalent, if they don't, that's an
   /// internal error.
   ///
   /// Returns: An expression referencing the variable.
   ///
   shared_ptr<hilti::expression::Variable> addTmp(const std::string& id, shared_ptr<Type> type, shared_ptr<hilti::Expression> init = nullptr, bool reuse = false, const Location& l = Location::None);

   /// Assocatiates a comment with the block. When the block is printed, the
   /// comment will be included.
   ///
   /// comment: The comment.
   void setBlockComment(const std::string& comment);

   /// Associates a comment with the next instruction or statement that will
   /// be added.
   ///
   /// comment: The comment.
   void addComment(const std::string& comment);

   /// Adds a statement at the current end of the block.
   ///
   /// stmt: The statement.
   void addInstruction(shared_ptr<Statement> stmt);

   /// Adds an instruction at the current end of the block. This version is
   /// for instructions \a without a target operand. The method is a helper
   /// that constructs the \a statement::Instruction instance out of the
   /// arguments and then call addStatement().
   ///
   /// ins: The instruction.
   ///
   /// op1: The first operand, if the instruction needs one.
   ///
   /// op2: The second operand, if the instruction needs one.
   ///
   /// op3: The third operand, if the instruction needs one.
   ///
   /// Returns: The new instruction that has been added to the block.
   shared_ptr<statement::Instruction> addInstruction(shared_ptr<hilti::Instruction> ins,
                                                     shared_ptr<hilti::Expression> op1 = nullptr,
                                                     shared_ptr<hilti::Expression> op2 = nullptr,
                                                     shared_ptr<hilti::Expression> op3 = nullptr,
                                                     const Location& l = Location::None
                                                     );

   /// Adds an instruction at the current end of the block. This version is
   /// for instructions \a with a target operand (though can be used for
   /// those without as well if \a target is set to null). The method is a
   /// helper that constructs the \a statement::Instruction instance out of
   /// the arguments and then call addStatement().
   ///
   /// target: The target operand, if the instruction needs one.
   ///
   /// ins: The instruction..
   ///
   /// op1: The first operand, if the instruction needs one.
   ///
   /// op2: The second operand, if the instruction needs one.
   ///
   /// op3: The third operand, if the instruction needs one.
   ///
   /// Returns: The new instruction that has been added to the block.
   shared_ptr<statement::Instruction> addInstruction(shared_ptr<hilti::Expression> target,
                                                     shared_ptr<hilti::Instruction> ins,
                                                     shared_ptr<hilti::Expression> op1 = nullptr,
                                                     shared_ptr<hilti::Expression> op2 = nullptr,
                                                     shared_ptr<hilti::Expression> op3 = nullptr,
                                                     const Location& l = Location::None
                                                     );

   /// Helper to add an \a if construct.
   ///
   /// cond: A boolean expression corresponding to the condition.
   ///
   /// Returns: A tuple corresponding to two blocks: (1) the one to execute
   /// if \a cond is true, and (2) the one where to continue execution in
   /// either case.
   std::tuple<shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>> addIf(shared_ptr<hilti::Expression> cond, const Location& l = Location::None);

   /// Helper to adds an \a if/else construct.
   ///
   /// cond: A boolean expression corresponding to the condition.
   ///
   /// Returns: A tuple corresponding to three blocks: (1) the one to execute
   /// if \a cond is true, (2) the one to execute if \a cond is true, and (3)
   /// the one where to continue execution in either case.
   std::tuple<shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>> addIfElse(shared_ptr<hilti::Expression> cond, const Location& l = Location::None);

   /// Helper to add a simple not-fmt debug message.
   ///
   /// stream: The stream to print the message to.
   ///
   /// msg: The message itself.
   void addDebugMsg(const std::string& stream, const std::string& msg);

   /// Helper to add a debug message with up to three format arguments.
   ///
   /// stream: The stream to print the message to.
   ///
   /// msg: The message itself.
   void addDebugMsg(const std::string& stream, const std::string& msg,
                 shared_ptr<hilti::Expression> arg1,
                 shared_ptr<hilti::Expression> arg2 = nullptr,
                 shared_ptr<hilti::Expression> arg3 = nullptr
                 );

   /// Helper to increase the debugging indent for a stream.
   ///
   /// stream: The stream to increase the level on.
   void debugPushIndent(const std::string& stream);

   /// Helper to decreate the debugging indent for a stream.
   ///
   /// stream: The stream to decreate the level on.
   void debugPopIndent(const std::string& stream);

protected:
   friend class ModuleBuilder;

   // Constructor.
   //
   // b: The block to wrap into the builder.
   BlockBuilder(shared_ptr<statement::Block> block, shared_ptr<statement::Block> body, ModuleBuilder* mbuilder);

   void setBody(shared_ptr<statement::Block> body) { _body = body; }

private:
   ModuleBuilder* _mbuilder;
   shared_ptr<hilti::statement::Block> _block_stmt;
   shared_ptr<hilti::expression::Block> _block_expr;
   shared_ptr<hilti::statement::Block> _body;
   std::list<std::string> _next_comments;
};

}

}

#endif
