
#include "visitor-interface.h"
#include "ast-info.h"

using namespace spicy;

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    ast::rtti::tryCast<spicy::Node>(node)->accept(this);
}
