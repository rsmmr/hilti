
#ifndef HILTI_CODEGEN_FIELDBUILDER_H
#define HILTI_CODEGEN_FIELDBUILDER_H

#include "../common.h"
#include "../visitor.h"

#include "codegen.h"
#include "common.h"

namespace hilti {
namespace codegen {

/// Visitor that generates the code for converting values into classifier
/// fields.
class FieldBuilder : public CGVisitor<llvm::Value*, shared_ptr<Type>, llvm::Value*> {
public:
    /// Constructor.
    ///
    /// cg: The code generator to use.
    FieldBuilder(CodeGen* cg);
    virtual ~FieldBuilder();

    /// Returns a LLVM value representing a classifier's field, intialized
    /// from a value.
    ///
    /// field_type: The type of the field; must of trait trait::Classifiable.
    ///
    /// src_type: The type of the value to initialize it with. The type must
    /// one of the ones that \a field_type's matchableTypes() returns.
    ///
    /// src_val: The value to initialize it with.
    ///
    /// Returns: A pointer to a ``hlt.classifier.field``. Passes ownership for
    /// the newly allocated object to the calling code.
    llvm::Value* llvmClassifierField(shared_ptr<Type> field_type, shared_ptr<Type> src_type,
                                     llvm::Value* src_val, const Location& l);

protected:
    const Location& location() const
    {
        return _location;
    }

    virtual void visit(type::Reference* t) override;
    virtual void visit(type::Address* t) override;
    virtual void visit(type::Network* t) override;
    virtual void visit(type::Port* t) override;
    virtual void visit(type::Bool* t) override;
    virtual void visit(type::Integer* t) override;
    virtual void visit(type::Bytes* t) override;

private:
    Location _location = Location::None;
};
}
}

#endif
