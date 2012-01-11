
#include "common.h"
#include "visitor-interface.h"

using namespace hilti;

VisitorInterface::~VisitorInterface()
{
}

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    std::dynamic_pointer_cast<hilti::Node>(node)->accept(this);
}

