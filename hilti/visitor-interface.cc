
#include "visitor-interface.h"
#include "common.h"

using namespace hilti;

VisitorInterface::~VisitorInterface()
{
}

void VisitorInterface::callAccept(shared_ptr<ast::NodeBase> node)
{
    ast::rtti::checkedCast<hilti::Node>(node.get())->accept(this);
}
