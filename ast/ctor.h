
#ifndef AST_CTOR_H
#define AST_CTOR_H

#include "exception.h"
#include "node.h"
#include "visitor.h"

namespace ast {

/// Base class for AST nodes representing constructed (but non-constant)
/// values..
template<typename AstInfo>
class Ctor : public AstInfo::node
{
public:
    typedef typename AstInfo::type Type;
    typedef typename AstInfo::visitor_interface VisitorInterface;

    // Constructor.
    //
    /// l: A location associated with the ctor.
    Ctor(const Location& l=Location::None)
       : AstInfo::node(l) {}

    /// Returns the type of the ctor. Must be overriden by derived
    /// classes.
    virtual shared_ptr<Type> type() const = 0;

    ACCEPT_DISABLED;
};

}

#endif
