
#ifndef HILTI_CODEGEN_LOADER_H
#define HILTI_CODEGEN_LOADER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

// Internal structure encapsulating the result of the Loader visitor.
struct _LoadResult
{
   llvm::Value* value; // XXX
   bool cctor;         // XXX
   bool is_ptr;        // XXX
};

/// Visitor that generates the code for loading the value of an HILTI
/// constant or expression. Note that this class should not be used directly,
/// the main frontend function is CodeGen::llvmValue().
class Loader : public CGVisitor<_LoadResult>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Loader(CodeGen* cg);
   virtual ~Loader();

   /// Returns the LLVM value resulting from evaluating a HILTI expression.
   /// The value will have its cctor function called already (if necessary)
   /// to create a new copy. If that isn't needed, llvmDtor() must be called
   /// on the result.
   ///
   /// expr: The expression to evaluate.
   ///
   /// cctor: If true, the value will be run through its cctor (where
   /// applicable) before returning it. The caller must then later run its
   /// dtor.
   ///
   /// coerce_to: If given, the expr is first coerced into this type before
   /// it's evaluated. It's assumed that the coercion is legal and supported.
   ///
   /// Returns: The computed LLVM value.
   llvm::Value* llvmValue(shared_ptr<Expression> expr, bool cctor, shared_ptr<hilti::Type> coerce_to = nullptr);

   /// Returns the LLVM value corresponding to a HILTI constant. The value
   /// will have its cctor function called already (if necessary) to create a
   /// new copy. If that isn't needed, llvmDtor() must be called on the
   /// result.
   ///
   /// cctor: If true, the value will be run through its cctor (where
   /// applicable) before returning it. The caller must then later run its
   /// dtor.
   ///
   /// constant: The constant.
   ///
   /// Returns: The computed LLVM value.
   llvm::Value* llvmValue(shared_ptr<Constant> constant, bool cctor);

   /// Returns the LLVM value corresponding to a HILTI constructor. The value
   /// will have its cctor function called already (if necessary) to create a
   /// new copy. If that isn't needed, llvmDtor() must be called on the
   /// result.
   ///
   /// constant: The constant.
   ///
   /// cctor: If true, the value will be run through its cctor (where
   /// applicable) before returning it. The caller must then later run its
   /// dtor.
   ///
   /// Returns: The computed LLVM value.
   llvm::Value* llvmValue(shared_ptr<Ctor> ctor, bool cctor);

protected:
   /// XXX
   void setResult(llvm::Value* _value, bool _cctor, bool _is_ptr) {
       _LoadResult result;
       result.value = _value;
       result.cctor = _cctor;
       result.is_ptr = _is_ptr;
       CGVisitor<_LoadResult>::setResult(result);
   }

   bool preferCctor() const { return _cctor; }
   bool preferPtr() const { return _cctor; }

   virtual void visit(expression::Variable* p) override;
   virtual void visit(expression::Parameter* p) override;
   virtual void visit(expression::CodeGen* c) override;

   virtual void visit(expression::Constant* e) override;
   virtual void visit(expression::Ctor* e) override;
   virtual void visit(expression::Coerced* e) override;
   virtual void visit(expression::Function* f) override;

   virtual void visit(variable::Local* v) override;
   virtual void visit(variable::Global* v) override;

   virtual void visit(constant::Integer* c) override;
   virtual void visit(constant::String* s) override;
   virtual void visit(constant::Bool* b) override;
   virtual void visit(constant::Label* l) override;
   virtual void visit(constant::Tuple* t) override;
   virtual void visit(constant::Reference* t) override;

   virtual void visit(ctor::Bytes* b) override;

private:
   // Cctors/dtors the result as necessary.
   llvm::Value* normResult(const _LoadResult& result, shared_ptr<Type> type, bool cctor);

   bool _cctor;
};


}
}

#endif
