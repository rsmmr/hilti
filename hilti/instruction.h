
#ifndef HILTI_INSTRUCTION_H
#define HILTI_INSTRUCTION_H

#include <vector>

#include "id.h"
#include "type.h"
#include "expression.h"

namespace hilti {

namespace statement { namespace instruction { class Resolved; } }

class InstructionRegistry;
extern unique_ptr<InstructionRegistry> theInstructionRegistry;

namespace instruction {

typedef std::vector<node_ptr<Expression>> Operands;
typedef std::vector<shared_ptr<Type>> Types;
typedef std::vector<const std::type_info*> TypeInfos;
typedef std::vector<bool> Flags;

}

namespace passes { class Validator; }

// Base class for defining a HILTI instruction. Note that one shouldn't
// derive directly from this class but use the "i-macros" in
// instructions/define-instruction.h instead. These macros apply some magic
// to override methods as needed.
class Instruction
{
public:
   typedef shared_ptr<statement::instruction::Resolved> (*stmt_factory)(
       shared_ptr<hilti::Instruction> instruction, const instruction::Operands& ops, const Location& l);

   /// Returns the name of the instruction.
   shared_ptr<ID> id() const { return _id; }

   /// Returns the instruction's factory function. The factory creates
   /// instances of Statement-derived classes corresponding to an instruction
   /// of this type with arguments bound.
   stmt_factory factory() const { return _factory; }

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

   ACCEPT_VISITOR_ROOT();

protected:
   friend class InstructionRegistry;

   /// Constructor.
   ///
   /// id: The name of instruction.
   ///
   /// factor: The factory function. See factory().
   Instruction(shared_ptr<ID> id, stmt_factory factory) {
       _id = id;
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
   bool matchesOperands(const instruction::Operands& ops) const;

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

   /// Checks whether an operand can be coerced into a given target type. If
   /// not, calls error(). This is primarily intended to be called from
   /// __validate().
   ///
   /// op: The operand to coercion for.
   ///
   /// dst: The type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> target) const;

   /// Checks whether an operand can be coerced into the type of a given
   /// target expression. If not, calls error(). This is primarily intended
   /// to be called from __validate().
   ///
   /// op: The operand to coercion for.
   ///
   /// dst: The type to coerce into.
   ///
   /// Returns: True if coercion is possible.
   bool canCoerceTo(shared_ptr<Expression> op, shared_ptr<Expression> target) const;

   /// Checks whether of two operands, one can be coerced into the other. It
   /// doesn't matter which one would be the source and which the
   /// destination. If coercion is not possible, calls error(). This is
   /// primarily intended to be called from __validate().
   ///
   /// op1: The first operand.
   ///
   /// op2: The second operand.
   ///
   /// Returns: True if coercion is possible either way.
   bool canCoerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const;

   // For internal use only. Will be overridden automagically via macros.
   virtual void __validate(const hilti::instruction::Operands& ops) const {}

   // For internal use only. Will be overridden automagically via macros.
   virtual const char* __doc() const { return "No documentation."; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp0(shared_ptr<Expression> op) const { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp1(shared_ptr<Expression> op) const { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp2(shared_ptr<Expression> op) const { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __matchOp3(shared_ptr<Expression> op) const { return op.get() == nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp0() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp1() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp2() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual shared_ptr<Expression> __defaultOp3() const { return nullptr; }

   // For internal use only. Will be overridden automagically via macros.
   virtual bool __terminator() const { return false; }

   node_ptr<ID> _id;
   stmt_factory _factory;
   passes::Validator* _validator = 0;
};


/// Singleton class that provides a register of all available HILTI
/// instructions.
class InstructionRegistry
{
public:
   InstructionRegistry() { addAll(); };
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

   /// Returns an instruction by name. The name must be unambigious. The
   /// methods aborts if it is not, or the name does not exist at all.
   ///
   /// name: The name of the instruction.
   ///
   /// Returns: The instruction.
   shared_ptr<Instruction> byName(const string& name) const;

   /// Returns a map of all instructions, indexed by their names.
   const instr_map& getAll() const { return _instructions; }

   /// Instantiates a statement::Instruction for an instruction/operand
   /// combination. Each instructions has its own class derived from
   /// statement::instruction::Resolved. This method instantiates a object of
   /// that class and initializes its operands with those of \a stmt.
   ///
   /// instr: The instruction to instantiate a statement for.
   ///
   /// stmts: The statement to take the operands from.
   shared_ptr<statement::instruction::Resolved> resolveStatement(shared_ptr<Instruction> instr, shared_ptr<statement::Instruction> stmt);


   /// Internal helper class to register an instruction with the registry.
   class Register
   {
   public:
      /// Constructor.
      ///
      /// ins: The instruction to register.
      Register(shared_ptr<Instruction> ins) { theInstructionRegistry->addInstruction(ins); }
   };

private:
   friend class Register;

   void addInstruction(shared_ptr<Instruction> ins);
   void addAll(); // This is implemented in instructions/all.cc

   instr_map _instructions;
};


}

#endif
