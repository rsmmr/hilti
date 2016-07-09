
#ifndef HILTI_CODEGEN_LOADER_H
#define HILTI_CODEGEN_LOADER_H

#include "../common.h"
#include "../visitor.h"

#include "codegen.h"
#include "common.h"

namespace hilti {
namespace codegen {

// Internal structure encapsulating the result of the Loader visitor.
struct _LoadResult {
    llvm::Value* value; // XXX
    bool cctor;         // XXX
    bool is_ptr;        // XXX
    bool stored_in_dst; // XXX
    bool is_hoisted;    // XXX
};

/// Visitor that generates the code for loading the value of an HILTI
/// constant or expression. Note that this class should not be used directly,
/// the main frontend function is CodeGen::llvmValue().
class Loader : public CGVisitor<_LoadResult, shared_ptr<Type>> {
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
    llvm::Value* llvmValue(shared_ptr<Expression> expr, bool cctor,
                           shared_ptr<hilti::Type> coerce_to = nullptr);

    /// Stores the LLVM value resulting from evaluating a HILTI expression
    /// into an address. The value will have its cctor function called already
    /// (if necessary) to create a new copy. If that isn't needed, llvmDtor()
    /// must be called on the result.
    ///
    /// dst: The address where to store the value.
    ///
    /// expr: The expression to evaluate.
    ///
    /// cctor: If true, the value will be run through its cctor (where
    /// applicable) before returning it. The caller must then later run its
    /// dtor.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    void llvmValueInto(llvm::Value* dst, shared_ptr<Expression> expr, bool cctor,
                       shared_ptr<hilti::Type> coerce_to = nullptr);

    /// Stores the LLVM value corresponding to a HILTI constant into an
    /// address. The value will have its cctor function called already (if
    /// necessary) to create a new copy. If that isn't needed, llvmDtor() must
    /// be called on the result.
    ///
    /// cctor: If true, the value will be run through its cctor (where
    /// applicable) before returning it. The caller must then later run its
    /// dtor.
    ///
    /// constant: The constant.
    ///
    /// Returns: The computed LLVM value.
    llvm::Value* llvmValue(shared_ptr<Constant> constant, bool cctor);

    /// Returns the LLVM value corresponding to a HILTI constant. The value
    /// will have its cctor function called already (if necessary) to create a
    /// new copy. If that isn't needed, llvmDtor() must be called on the
    /// result.
    ///
    /// dst: The address where to store the value.
    ///
    /// cctor: If true, the value will be run through its cctor (where
    /// applicable) before returning it. The caller must then later run its
    /// dtor.
    ///
    /// constant: The constant.
    void llvmValueInto(llvm::Value* dst, shared_ptr<Constant> constant, bool cctor);

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

    /// Store the LLVM value corresponding to a HILTI constructor into an
    /// address. The value will have its cctor function called already (if
    /// necessary) to create a new copy. If that isn't needed, llvmDtor() must
    /// be called on the result.
    ///
    /// dst: The address where to store the value.
    ///
    /// constant: The constant.
    ///
    /// cctor: If true, the value will be run through its cctor (where
    /// applicable) before returning it. The caller must then later run its
    /// dtor.
    void llvmValueInto(llvm::Value* dst, shared_ptr<Ctor> ctor, bool cctor);

    /// Returns the address of a value referenced by an expression, if possible.
    /// For types that don't support taking their address, returns null.
    ///
    /// expr: The expression to evaluate.
    llvm::Value* llvmValueAddress(shared_ptr<Expression> expr);

protected:
    /// XXX
    void setResult(llvm::Value* _value, bool _cctor, bool _is_ptr, bool _stored_in_dst = false,
                   bool _is_hoisted = false)
    {
        _LoadResult result;
        result.value = _value;
        result.cctor = _cctor;
        result.is_ptr = _is_ptr;
        result.stored_in_dst = _stored_in_dst;
        result.is_hoisted = _is_hoisted;
        CGVisitor<_LoadResult, shared_ptr<Type>>::setResult(result);
    }

    bool preferCctor() const
    {
        return _cctor;
    }
    bool preferPtr() const
    {
        return _cctor;
    }

    virtual void visit(expression::CodeGen* c) override;
    virtual void visit(expression::Coerced* e) override;
    virtual void visit(expression::Constant* e) override;
    virtual void visit(expression::Ctor* e) override;
    virtual void visit(expression::Default* e) override;
    virtual void visit(expression::Function* e) override;
    virtual void visit(expression::Parameter* e) override;
    virtual void visit(expression::Variable* e) override;
    virtual void visit(expression::Type* e) override;

    virtual void visit(variable::Global* v) override;
    virtual void visit(variable::Local* v) override;

    virtual void visit(constant::Address* c) override;
    virtual void visit(constant::Bitset* c) override;
    virtual void visit(constant::Bool* b) override;
    virtual void visit(constant::CAddr* b) override;
    virtual void visit(constant::Double* c) override;
    virtual void visit(constant::Enum* c) override;
    virtual void visit(constant::Integer* c) override;
    virtual void visit(constant::Interval* c) override;
    virtual void visit(constant::Label* l) override;
    virtual void visit(constant::Network* c) override;
    virtual void visit(constant::Port* c) override;
    virtual void visit(constant::Reference* c) override;
    virtual void visit(constant::String* c) override;
    virtual void visit(constant::Time* c) override;
    virtual void visit(constant::Tuple* c) override;
    virtual void visit(constant::Unset* c) override;
    virtual void visit(constant::Union* c) override;

    virtual void visit(ctor::Bytes* c) override;
    virtual void visit(ctor::List* c) override;
    virtual void visit(ctor::Map* c) override;
    virtual void visit(ctor::RegExp* c) override;
    virtual void visit(ctor::Set* c) override;
    virtual void visit(ctor::Vector* c) override;
    virtual void visit(ctor::Callable* c) override;

private:
    // Cctors/dtors the result as necessary.
    llvm::Value* normResult(const _LoadResult& result, shared_ptr<Type> type, bool cctor,
                            llvm::Value* dst);

    bool _cctor;
    llvm::Value* _dst;
};
}
}

#endif
