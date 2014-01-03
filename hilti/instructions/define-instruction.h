
#ifndef HILTI_IMPLEMENT_INSTRUCTION_H
#define HILTI_IMPLEMENT_INSTRUCTION_H

// This header defines the macros we use to specify HILTI instructions. It's
// pretty ugly ...

#include "../instruction.h"
#include "../statement.h"
#include "../builder/nodes.h"

#include "optypes.h"

#define __cat(a,b) a##b
#define __unique(l) __cat(register_, l)

/// Registers an instruction with the InstructionRegistry.
#define IMPLEMENT_INSTRUCTION(ns, cls) \
   void __register_##ns##_##cls() { \
        ::hilti::instruction::ns::cls = std::make_shared<::hilti::instruction::ns::__class::cls>(); \
        InstructionRegistry::globalRegistry()->addInstruction(::hilti::instruction::ns::cls); \
   }

/// Begins the definition of an instruction. Old version; use iBeginH/iBeginCC instead.
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
                : instruction::Resolved(instruction, ops, l) {} \
            ACCEPT_VISITOR(instruction::Resolved)   \
        };                                          \
       }                                            \
      }                                             \
     }                                              \
                                                    \
     namespace instruction {                        \
       namespace ns {                               \
         namespace __class {                        \
                                                    \
          class cls : public hilti::Instruction     \
   {                                                \
   public:                                          \
       static shared_ptr<statement::instruction::Resolved> factory(shared_ptr<hilti::Instruction> instruction, \
           const hilti::instruction::Operands& ops, \
           const Location& l=Location::None) {      \
           return shared_ptr<statement::instruction::Resolved>(new typename hilti::statement::instruction::ns::cls(instruction, ops, l)); \
           }                                        \
       cls() : hilti::Instruction(shared_ptr<ID>(new ID(name)), #ns, #cls, factory) {} \
       virtual ~cls() {}


/// Ends the definition of an instruction. Old version - use iEndH/iEndC instead.
#define iEnd  \
   }          \
   };         \
   }          \
   }          \
   }          \
   }

/// Begins the definition of an instruction. Old version; use iBeginH/iBeginCC instead.
///
/// ns: The namespace to use for the statement class. The class will be in \c
/// hilti::statement::instruction::<ns>.
///
/// cls: The name of the statement class. The class will be \c
/// hilti::statement::instruction::<ns>::<cls>.
///
/// name: The source level name of the instruction (e.g., "string.concat").
#define iBeginH(ns, cls, name)                       \
   iBegin(ns, cls, name);                            \
   const char* __doc() const override;               \
   void __validate(const hilti::instruction::Operands& ops) const;

#define iEndH \
   };         \
   }          \
   }          \
   }          \
   }

#define iBeginCC(ns)                                \
   namespace hilti {                                \
     namespace instruction {                        \
      namespace ns {                                \
       namespace __class {                          \

#define iEndCC \
   }          \
   }          \
   }          \
   }

#define __implementOp(nr, ty, constant) \
       shared_ptr<Type> __typeOp##nr()  const override { \
           return ty;                                    \
       }                                                 \
                                                         \
       bool __matchOp##nr(shared_ptr<Expression> op, bool coerce) override {        \
           if ( ! op ) return __defaultOp##nr() || ast::isA<type::OptionalArgument>(ty); \
           if ( coerce ) {                                             \
               if ( ! op->canCoerceTo(ty) ) return false;              \
           }                                                           \
           else                                                        \
               if ( ! ty->equal(op->type()) ) return false;            \
           if ( op->isConstant() && ! constant ) return false;         \
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
#define iDefault2(def)  __implementDefault(2, def)

/// Defines a default for an instruction's 3st operand.
///
/// def: The default Expression for the operand.
#define iDefault3(def)  __implementDefault(3, def)

#define __get_op(n) \
    ops[n] ? ops[n]->sharedPtr<Expression>() : shared_ptr<Expression>();

/// Starts a C++ code block that verifies the correctess of an instruction's
/// operands. The code has access to Expression instances \a target, \a op1,
/// \a op2, \a op3, that represent the Expression for the corresponding
/// operands.
///
/// Deprecated. Use iValidateCC instead.
#define iValidate \
       void __validate(const hilti::instruction::Operands& ops) const override { \
           shared_ptr<Expression> target = __get_op(0); \
           shared_ptr<Expression> op1 = __get_op(1); \
           shared_ptr<Expression> op2 = __get_op(2); \
           shared_ptr<Expression> op3 = __get_op(3); \

/// Starts a C++ code block that verifies the correctess of an instruction's
/// operands. The code has access to Expression instances \a target, \a op1,
/// \a op2, \a op3, that represent the Expression for the corresponding
/// operands.
///
/// cls: The class name for the instruction.
#define iValidateCC(cls) \
       void cls::__validate(const hilti::instruction::Operands& ops) const { \
           shared_ptr<Expression> target = __get_op(0); \
           shared_ptr<Expression> op1 = __get_op(1); \
           shared_ptr<Expression> op2 = __get_op(2); \
           shared_ptr<Expression> op3 = __get_op(3); \

/// Defines the manual entry for an instruction, describing its semantics and
/// operands.
///
/// txt: A string with the text for the manual.
///
/// Deprecated. Use iValidateCC instead.
#define iDoc(txt) \
       } \
       const char* __doc() const override { { return txt; }

/// Defines the manual entry for an instruction, describing its semantics and
/// operands.
///
/// cls: The class name for the instruction.
///
/// txt: A string with the text for the manual.
#define iDocCC(cls, txt) \
       } \
     const char* cls::__doc() const { return txt; }


/// Marks an instruction as a block terminator.
#define iTerminator() \
       std::set<shared_ptr<Expression>> __successors(const hilti::instruction::Operands& ops) const override; \
       bool __terminator() const override { return true; }

/// Returns the successors of a terminator instructions.
#define iSuccessorsCC(cls) \
       } \
       std::set<shared_ptr<Expression>> cls::__successors(const hilti::instruction::Operands& ops) const { \
           shared_ptr<Expression> target = __get_op(0); \
           shared_ptr<Expression> op1 = __get_op(1); \
           shared_ptr<Expression> op2 = __get_op(2); \
           shared_ptr<Expression> op3 = __get_op(3); \

/// Override flowInfo().
#define iFlowInfoH() \
       FlowInfo __flowInfo(FlowInfo fi) override;

#define iFlowInfoCC(cls) \
       } \
       FlowInfo cls::__flowInfo(FlowInfo fi) {

#endif
