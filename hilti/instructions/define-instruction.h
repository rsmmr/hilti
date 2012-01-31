
#ifndef HILTI_IMPLEMENT_INSTRUCTION_H
#define HILTI_IMPLEMENT_INSTRUCTION_H

// This header defines the macros we use to specify HILTI instructions. It's
// pretty ugly ...

#include "../instruction.h"
#include "../statement.h"

#include "optypes.h"

#define __cat(a,b) a##b
#define __unique(l) __cat(register_, l)

/// Registers an instruction with the InstructionRegistry.
#define implementInstruction(cls) \
   addInstruction(shared_ptr<Instruction>(new typename instruction::cls));

/// Begins the definition of an instruction.
///
/// ns: The namespace to use for the statement class. The class will be in \c
/// hilti::statement::instruction::<ns>.
///
/// cls: The name of the statement class. The class will be \c
/// hilti::statement::instruction::<ns>::<cls>.
/// 
/// name: The source level name of the instruction (e.g., "string.concat").
#define iBegin(ns, cls, name)                       \
   namespace hilti {                                \
     namespace statement {                          \
      namespace instruction {                       \
       namespace ns {                               \
        class cls : public instruction::Resolved {   \
          public:                                   \
             cls(shared_ptr<hilti::Instruction> instruction, \
                 const hilti::instruction::Operands& ops,  \
                 const Location& l=Location::None)  \
                : instruction::Resolved(instruction, ops) {} \
            ACCEPT_VISITOR(instruction::Resolved)   \
        };                                          \
       }                                            \
      }                                             \
     }                                              \
                                                    \
     namespace instruction {                        \
       namespace ns {                               \
                                                    \
   class cls : public Instruction                   \
   {                                                \
   public:                                          \
       static shared_ptr<statement::instruction::Resolved> factory(shared_ptr<hilti::Instruction> instruction, \
           const hilti::instruction::Operands& ops, \
           const Location& l=Location::None) {      \
           return shared_ptr<statement::instruction::Resolved>(new typename hilti::statement::instruction::ns::cls(instruction, ops, l)); \
           }                                        \
       cls() : Instruction(shared_ptr<ID>(new ID(name)), factory) {}


/// Ends the definition of an instruction.
#define iEnd  \
   }          \
   };         \
   }          \
   }          \
   }

#define __implementOp(nr, ty, constant) \
       bool __matchOp##nr(shared_ptr<Expression> op) const override {  \
           if ( ! op ) return false;                                   \
           if ( ! ty->equal(op->type()) ) return false;                        \
           if ( op->isConstant() && ! constant ) return false;         \
           if ( ! op && ! __defaultOp##nr() ) return false;            \
           return true;                                                \
           }

#define __implementDefault(op, def) \
       shared_ptr<Expression> __default##op() const override {                                     \
           return shared_ptr<Expression>(new expression::Constant(shared_ptr<Constant>(new def))); \
       }

/// Defines the type of an instruction's target.
///
/// ty: The Type of the target.
#define iTarget(ty)         __implementOp(0, ty, false)

/// Defines the type of an instructions 1st operand.
///
/// ty: The Type of the operand.
///
/// constant: True if the operand is declared as constant.
#define iOp1(ty, constant)  __implementOp(1, ty, constant)

/// Defines the type of an instructions 2nd operand.
///
/// ty: The Type of the operand.
///
/// constant: True if the operand is declared as constant.
#define iOp2(ty, constant)  __implementOp(2, ty, constant)

/// Defines the type of an instructions 3rd operand.
///
/// ty: The Type of the operand.
///
/// constant: True if the operand is declared as constant.
#define iOp3(ty, constant)  __implementOp(3, ty, constant)

/// Defines a default for an instruction's 1st operand.
///
/// def: The default Expression for the operand. 
#define iDefault1(def)  __implementDefault(1, def)

/// Defines a default for an instruction's 2st operand.
///
/// def: The default Expression for the operand. 
#define iDefault2(def)  __implementDdefault(2, def)

/// Defines a default for an instruction's 3st operand.
///
/// def: The default Expression for the operand. 
#define iDefault3(def)  __implementDdefault(3, def)

#define __get_op(n) \
    ops[n] ? ops[n]->sharedPtr<Expression>() : shared_ptr<Expression>();

/// Starts a C++ code block that verifies the correctess of an instruction's
/// operands. The code has access to Expression instances \a target, \a op1,
/// \a op2, \a op3, that represent the Expression for the corresponding
/// operands.
#define iValidate \
       void __validate(const hilti::instruction::Operands& ops) const override { \
           shared_ptr<Expression> target = __get_op(0); \
           shared_ptr<Expression> op1 = __get_op(1); \
           shared_ptr<Expression> op2 = __get_op(2); \
           shared_ptr<Expression> op3 = __get_op(3); \

/// Defines the manual entry for an instruction, describing its semantics and
/// operands.
///
/// txt: A string with the text for the manual.
#define iDoc(txt) \
       } \
       const char* __doc() const override { { return txt; }

#endif
