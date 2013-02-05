
#include "flow-info.h"

using namespace hilti;

bool FlowVariablePtrCmp::operator()(const shared_ptr<FlowVariable>& f1, const shared_ptr<FlowVariable>& f2)
{
    return f1->name < f2->name;
}

bool hilti::operator==(const shared_ptr<FlowVariable>& v1, const shared_ptr<FlowVariable>& v2)
{
    return v1->name == v2->name;
}
