
#ifndef BINPAC_CODEGEN_COERCION_BUILDER_H
#define BINPAC_CODEGEN_COERCION_BUILDER_H

#include "common.h"
#include "cg-visitor.h"

namespace binpac {
namespace codegen {

/// Visitor that generates code for coercing expressions from one type to another.
///
/// Note that this class should not be used directly, the CodeGen provides a
/// frontend method instead.
class CoercionBuilder : public CGVisitor<shared_ptr<hilti::Expression>, shared_ptr<hilti::Expression>, shared_ptr<Type>>
{
public:
    /// Constructor.
    ///
    /// cg: The code generator to use.
    CoercionBuilder(CodeGen* cg);
    virtual ~CoercionBuilder();

    /// Coerces a HILTI expression of one BinPAC type into another. The
    /// method assumes that the coercion is legel.
    ///
    /// expr: The HILTI expression to coerce.
    ///
    /// src: The BinPAC type that \a expr currently has.
    ///
    /// dst: The BinPAC type that \a expr is to be coerced into.
    ///
    /// Returns: The coerced expression.
    shared_ptr<hilti::Expression> hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst);

protected:
   void visit(type::Integer* i);
};

}
}

#endif
