
#ifndef AST_STATEMENT_H
#define AST_STATEMENT_H

#include "node.h"
#include "constant.h"

namespace ast {

/// Base class for AST nodes representing statements.
template<typename AstInfo>
class Statement : public AstInfo::node
{
public:
    typedef typename AstInfo::node Node;

    /// Constructor.
    ///
    /// l: Associated location.
    Statement(const Location& l=Location::None) : AstInfo::node(l) {}
};

}

#endif
