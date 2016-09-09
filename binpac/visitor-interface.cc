
#include "visitor-interface.h"
#include "ast-info.h"

using namespace binpac;

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    ast::rtti::tryCast<binpac::Node>(node)->accept(this);
}
