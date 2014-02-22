
#ifndef HILTI_INSTRUCTION_H
#define HILTI_INSTRUCTION_H

#include <vector>

#include "id.h"
#include "type.h"
#include "expression.h"
#include "flow-info.h"

namespace hilti {

namespace statement { namespace instruction { class Resolved; } }

namespace instruction {

typedef std::vector<node_ptr<Expression>> Operands;
typedef std::vector<shared_ptr<Type>> Types;
typedef std::vector<const std::type_info*> TypeInfos;
typedef std::vector<bool> Flags;

}

namespace passes { class Validator; }

/// A class providing a number of helper methods for implementing
/// instructions. We derive from this class in different subclasses doing
/// instruction handling.
class InstructionHelper
{
public:
   /// For an expression of type type::Type, returns the corresponding
   /// typeType(). Passing an expression of a different type is an error.
   ///
   /// op: The expression of type type::Type.
   ///
   /// Returns: The type's type.
   shared_ptr<Type> typedType(shared_ptr<Expression> op) const;

   /// For an expression of type type::Reference, returns the corresponding
   /// referenceType(). Passing an expression of a different type is an
   /// error.
   ///
   /// op: The expression of type type::Reference.
   ///
   /// Returns: The type's type.
   shared_ptr<Type> referencedType(shared_ptr<Expression> op) const;

   /// For a type type::Reference, returns the corresponding referenceType().
   /// Passing an expression of a different type is an error.
   ///
   /// ty: The type::Reference.
   ///
   /// Returns: The type's type.
   shared_ptr<Type> referencedType(shared_ptr<Type> ty) const;

   /// For an expression of type type::Container or type::Reference pointing
   /// to a container, returns the corresponding argType(). Passing an
   /// expression of a different type is an error.
   ///
   /// op: The expression of type type::Iterator.
   ///
   /// Returns: The type's type.
   ///
   /// \todo This is deprecated and supersed by argType(), which will work
   /// for containers too.
   shared_ptr<Type> elementType(shared_ptr<Expression> op) const;

   /// For a type of type type::Container or type::Reference pointing to a
   /// container, returns the corresponding argType(). Passing an expression
   /// of a different type is an error.
   ///
   /// op: The expression of type type::Iterator.
   ///
   /// Returns: The type's type.
   ///
   /// \todo This is deprecated and supersed by argType(), which will work
   /// for containers too.
   shared_ptr<Type> elementType(shared_ptr<Type> ty) const;

   /// For an expression of type type::TypedHeapType or type::TypedValueType
   /// (or type::Reference pointing to one of either), returns the
   /// corresponding argType(). Passing an expression of a different type is
   /// an error.
   ///
   /// op: The expression of type type::Iterator.
   ///
   /// Returns: The type's type.
   shared_ptr<Type> argType(shared_ptr<Expression> op) const;

   /// For a type of type type::TypedHeapType or type::TypedValueType or
   /// (or type::Reference pointing to one of either), returns the
   /// argType(). Passing an expression of a different type is an error.
   ///
   /// op: The expression of type type::Iterator.
   ///
   /// Returns: The type's type.
   shared_ptr<Type> argType(shared_ptr<Type> ty) const;

   /// For an expression of type type::Iterator, returns the corresponding
   /// argType(). Passing an expression of a different type is an error.
   ///
   /// op: The expression of type type::Iterator.
   ///
   /// Returns: The type being iterated over.
   shared_ptr<Type> iteratedType(shared_ptr<Expression> op) const;

   /// For a a type type::Iterator, returns the corresponding argType().
   /// Passing an expression of a different type is an error.
   ///
   /// ty: The iterator type.
   ///
   /// Returns: The type being iterated over.
   shared_ptr<Type> iteratedType(shared_ptr<Type> ty) const;

   /// For an expression of type type::Map (or a reference to it), returns
   /// the corresponding keyType(). Passing an expression of a different type
   /// is an error.
   ///
   /// op: The expression of type type::Map.
   ///
   /// Returns: The key type.
   shared_ptr<Type> mapKeyType(shared_ptr<Expression> op) const;

   /// For a type type::Map (or a reference to it), returns the corresponding
   /// keyType(). Passing an expression of a different type is an error.
   ///
   /// ty: The type type::Map type.
   ///
   /// Returns: The key type.
   shared_ptr<Type> mapKeyType(shared_ptr<Type> ty) const;

   /// For an expression of type type::Map (or a reference to it), returns the corresponding
   /// valueType(). Passing an expression of a different type is an error.
   ///
   /// op: The expression of type type::Map.
   ///
   /// Returns: The value type.
   shared_ptr<Type> mapValueType(shared_ptr<Expression> op) const;

   /// For a type type::Map (or a reference to it), returns the corresponding
   /// valueType(). Passing an expression of a different type is an error.
   ///
   /// ty: The type type::Map type.
   ///
   /// Returns: The value type.
   shared_ptr<Type> mapValueType(shared_ptr<Type> ty) const;

};

/// Base class for defining a HILTI instruction. Note that one shouldn't
/// derive directly from this class but use the "i-macros" in
/// instructions/define-instruction.h instead. These macros apply some magic
/// to override methods as needed.
class Instruction : public InstructionHelper
{
public:
   typedef shared_ptr<statement::instruction::Resolved> (*stmt_factory)(
       shared_ptr<hilti::Instruction> instruction, const instruction::Operands& ops, const Location& l);

   virtual ~Instruction() {}

   /// Returns the name of the instruction.
   shared_ptr<ID> id() const { return _id; }

   /// Returns the instruction's factory function. The factory creates
   /// instances of Statement-derived classes corresponding to an instruction
   /// of this type with arguments bound.
   stmt_factory factory() const { return _factory; }

   /// Returns the current validator.
   passes::Validator* validator() const { return _validator; }

   /// Validates whether a set of arguments are valid for the instruction.
   /// Errors are reported via the given passes::Validator instance (which is
   /// also normally the one calling this method).
   ///
   /// v: The validator pass.
   ///
   /// ops: The operands to check.
   void validate(passes::Validator* v, const hilti::instruction::Operands& ops) {
       _validator = v;
       __validate(ops);
       _validator = nullptr;
   }

   /// Returns true if the instruction is a block terminator.
   bool terminator() const { return __terminator(); }

   /// Returns the instructions successors blocks. For non-terminators, it
   /// will always returns an empty set.
   ///
   /// ops: The operands to use,
   ///
   /// scope: The scope to look blocks up in.
   ///
   /// Returns: A set of successor blocks.
   std::set<shared_ptr<statement::Block>> successors(const hilti::instruction::Operands& ops, shared_ptr<Scope> scope) const;

   /// Returns the type of operand 1-3. The first element is the type itself,
   /// and the second indicates whether the operand is constant (true) or not
   /// (false). Returns \c (null, false) if the operand isn't used.
   ///
   /// \todo: Right now \a constant will always be false as we don't honor
   /// yet what the instruction definition specifies.
   std::pair<shared_ptr<Type>, bool> typeOperand(int n) const;

   /// Returns the type of the target, or null if no target.
   shared_ptr<Type> typeTarget() const;

   operator string() const;

   /// Method to report errors found by validate(). This forwards to the
   /// current passes::Validator.
   ///
   /// op: The AST node triggering the error.
   ///
   /// msg: An error message for the user.
   void error(Node* op, string msg) const;

   /// Method to report errors found by validate(). This forwards to the
   /// current passes::Validator.
   ///
   /// op: The AST node triggering the error.
   ///
   /// msg: An error message for the user.
   void error(shared_ptr<Node> op, string msg) const {
       return error(op.get(), msg);
   }

   /// Checks whether an operand can be coerced into a given target type.
   /// 
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op: The operand to coercion for.
   ///
   /// dst: The type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> target) const;

   /// Checks whether an operand can be coerced into the type of a given
   /// target expression.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op: The operand to coercion for.
   ///
   /// dst: The type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Expression> op, shared_ptr<Expression> target) const;

   /// Checks whether a source target can be coerced into the type of a given
   /// target expression.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// src: The source type.
   ///
   /// dst: The operand which's type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Type> src, shared_ptr<Expression> target) const;

   /// Checks whether a source target can be coerced into the type of a given
   /// target expression.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// src: The source type.
   ///
   /// dst: The type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> target) const;

   /// Checks whether of two operands, one can be coerced into the other. It
   /// doesn't matter which one would be the source and which the
   /// destination.
   /// 
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op1: The first operand.
   ///
   /// op2: The second operand.
   ///
   /// Returns: True if coercion is possible either way.
   bool canCoerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const;

   /// Checks whether an expression's type matches a given one.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op: The expression.
   ///
   /// target: The type *op* must match.
   ///
   /// Returns: True if the types match.
   bool hasType(shared_ptr<Expression> op, shared_ptr<Type> target) const;

   /// Checks whether two types are equal.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// ty1: The first type.
   /// 
   /// ty2: The second type.
   ///
   /// Returns: True if they are equal.
   bool equalTypes(shared_ptr<Type> ty1, shared_ptr<Type> ty2) const;

   /// Checks whether the types of two operands are equal.
   /// 
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op1: The first type.
   /// 
   /// op2: The second type.
   ///
   /// Returns: True if they are equal.
   bool equalTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const;

   /// Checks whether an epxression is a constant.
   ///
   /// If the method fails, it calls error() with a suitable error message.
   /// The method is primarily intended to be called from __validate().
   ///
   /// op: The operand.
   ///
   /// Returns: True if the expression is an instance of expression::Constant.
   bool isConstant(shared_ptr<Expression> op) const;

   /// Checks whether call parameters are compatible with a function. If not,
   /// an error is reported.
   bool checkCallParameters(shared_ptr<type::Function> func, shared_ptr<Expression> args, bool allow_unbound = false) const;

   /// Checks whether a return expression is compatible with a function's
   /// return type.
   bool checkCallResult(shared_ptr<Type> rtype, shared_ptr<Expression> expr) const;

   /// Checks whether a return type is compatible with a function's prototype.
   bool checkCallResult(shared_ptr<Type> rtype, shared_ptr<Type> ty) const;

   struct Info {
       string mnemonic;              /// The instructions mnemonic in the HILTI IR representation.
       string namespace_;            /// The namespace the instruction is part of.
       string class_;                /// The name of the instructions class (without the namespace).
       string description;           /// Textual description of the instruction.
       bool terminator;              /// True if this function is a block terminator.
       shared_ptr<Type> type_target; /// Type of the target, or null if not used.
       shared_ptr<Type> type_op1;    /// Type of operand 1, or null if not used.
       shared_ptr<Type> type_op2;    /// Type of operand 2, or null if not used.
       shared_ptr<Type> type_op3;    /// Type of operand 3, or null if not used.
       bool const_op1;               /// True if operand 1 won't be modified.
       bool const_op2;               /// True if operand 2 won't be modified.
       bool const_op3;               /// True if operand 3 won't be modified.
       shared_ptr<Expression> default_op1; /// Default for operand 1, or null if not used.
       shared_ptr<Expression> default_op2; /// Default for operand 2, or null if not used.
       shared_ptr<Expression> default_op3; /// Default for operand 3, or null if not used.
   };

   /// Returns information about the instruction for use in the reference documentation.
   Info info() const;

   /// Adapts the default flow information to instruction-specific constraints.
   FlowInfo flowInfo(FlowInfo fi) { return __flowInfo(fi); }

   ACCEPT_VISITOR_ROOT();

protected:
   friend class InstructionRegistry;

   /// Constructor.
   ///
   /// id: The name of instruction.
   ///
   /// factor: The factory function. See factory().
   Instruction(shared_ptr<ID> id, string namespace_, string class_, stmt_factory factory) {
       _id = id;
       _namespace = namespace_;
       _class = class_;
       _factory = factory;
   }

   /// Examines a set of operands for overload resolution.
   /// InstructionRegistry::getMatching() calls this method for overload
   /// resolution to see if a set of operands is compatible with the
   /// instruction's signature. Note that this does just simple type-checks.
   /// More fine-granular error checks are done by validate() once an
   /// instruction has been selected as the match.
   ///
   /// ops: The operands to match the signature against.
   ///
   /// Returns: True if the operands match the instruction's signature.
   bool matchesOperands(const instruction::Operands& ops, bool coerce);

   // For internal use only. Will be overridden automagically via macros.
   virtual void __validate(const hilti::instruction::Operands& ops) const {}

   // For internal use only. Will be overridden automagically via macros.
   virtual const char* __doc() const { return "No documentation."; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp0(shared_ptr<Expression> op, bool coerce) { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp1(shared_ptr<Expression> op, bool coerce) { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp2(shared_ptr<Expression> op, bool coerce) { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp3(shared_ptr<Expression> op, bool coerce) { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual std::pair<shared_ptr<Type>, bool> __typeOp0() const  { return std::make_pair(nullptr, false); }

   // For internal use only. Will be overridden automagically via macros.
   virtual std::pair<shared_ptr<Type>, bool> __typeOp1() const  { return std::make_pair(nullptr, false); }

   // For internal use only. Will be overridden automagically via macros.
   virtual std::pair<shared_ptr<Type>, bool> __typeOp2() const  { return std::make_pair(nullptr, false); }

   // For internal use only. Will be overridden automagically via macros.
   virtual std::pair<shared_ptr<Type>, bool> __typeOp3() const { return std::make_pair(nullptr, false); }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp0() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp1() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp2() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp3() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual std::set<shared_ptr<Expression>> __successors(const hilti::instruction::Operands& ops) const { return std::set<shared_ptr<Expression>>(); }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __terminator() const { return false; }

   // For internal use only. May adapt the flowinfo.
   virtual FlowInfo __flowInfo(FlowInfo fi) { return fi; };

   node_ptr<ID> _id;
   string _namespace;
   string _class;
   stmt_factory _factory;
   passes::Validator* _validator = 0;

};


/// Singleton class that provides a register of all available HILTI
/// instructions.
class InstructionRegistry
{
public:
   InstructionRegistry() {};
   ~InstructionRegistry() {}

   typedef std::list<shared_ptr<Instruction>> instr_list;
   typedef std::multimap<string, shared_ptr<Instruction>> instr_map;

   /// Returns true if at least one instruction of a given name exists.
   ///
   /// name: The name to look for.
   bool has(string name) const { return _instructions.find(name) != _instructions.end(); }

   /// Returns a list of instructions matching a name and a set of operands.
   /// This is the main function for overload resolution.
   ///
   /// id: The name to look for.
   ///
   /// ops: The operands to match all the instructions with the given name for.
   ///
   /// Returns: A list of instructions compatatible with \a id and \a ops.
   instr_list getMatching(shared_ptr<ID> id, const instruction::Operands& ops) const;

   /// Returns all instruction of a given name.
   ///
   /// name: The name of the instruction.
   ///
   /// Returns: The instruction.
   instr_list byName(const string& name) const;

   /// Returns a map of all instructions, indexed by their names.
   const instr_map& getAll() const { return _instructions; }

   /// Instantiates a statement::Instruction for an instruction/operand
   /// combination. Each instructions has its own class derived from
   /// statement::instruction::Resolved. This method instantiates a object of
   /// that class and initializes its operands with those of \a stmt. The
   /// methods coerces the resolved statements' operands to the types
   /// requested by the instruction's signature.
   ///
   /// instr: The instruction to instantiate a statement for.
   ///
   /// stmts: The statement to take the operands from.
   shared_ptr<statement::instruction::Resolved> resolveStatement(shared_ptr<Instruction> instr, shared_ptr<statement::Instruction> stmt);

   /// Instantiates a statement::Instruction for an instruction/operand
   /// combination. Each instructions has its own class derived from
   /// statement::instruction::Resolved. This method instantiates a object of
   /// that class and initializes its operands with those given. The methods
   /// coerces the resolved statements' operands to the types requested by
   /// the instruction's signature.
   ///
   /// instr: The instruction to instantiate a statement for.
   ///
   /// ops: The operands.
   shared_ptr<statement::instruction::Resolved> resolveStatement(shared_ptr<Instruction> instr, const instruction::Operands& ops, const Location& l = Location::None);

   /// Registers an instruction with the registry. This will be called from
   /// the IMPLEMENT_INSTRUCTION macro.
   void addInstruction(shared_ptr<Instruction> ins);

   /// Return the singleton for the global instruction registry.
   static InstructionRegistry* globalRegistry();

private:
   instr_map _instructions;

   static InstructionRegistry* _registry; // Singleton.
};

}

#endif
