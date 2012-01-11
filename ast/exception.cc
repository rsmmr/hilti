
#include "exception.h"

using namespace ast;

const char* Exception::what() const throw() {
    string what = _what + " [";

    if ( _node )
        what += string(_node->location());
    else
        what += "no node";

    what += "]";

    return what.c_str();
}
