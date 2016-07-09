
#include "visitor-interface.h"
#include "ast-info.h"

using namespace binpac;

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    dynamicPointerCast(node)->accept(this, binpac::Node);
}
