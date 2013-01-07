
#include "visitor.h"

namespace ast {

bool _debug_all_visitors = false;

void enableDebuggingForAllVisitors(bool enabled)
{
    _debug_all_visitors = enabled;
}

bool debuggingAllVisitors()
{
    return _debug_all_visitors;
}

}
