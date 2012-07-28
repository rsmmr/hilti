
#include "ast-info.h"
#include "visitor-interface.h"

using namespace binpac;

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    std::dynamic_pointer_cast<binpac::Node>(node)->accept(this);
}

