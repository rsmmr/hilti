
#include <util/util.h>

#include "location.h"

using namespace ast;

const Location Location::None("<no location>");

Location::operator string() const
{
    if ( this == &None )
        return "<no location>";

    string s = _file.size() ? _file : "<no filename>";
    s += ":";

    if ( _from >= 0 ) {
        if ( _to >= 0 )
            s += util::fmt("%d-%d", _from, _to);
        else
            s += util::fmt("%d", _from);
    }

    return s;
}
