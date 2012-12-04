
#include "coercer.h"

using namespace binpac;

void Coercer::visit(type::Integer* i)
{
    setResult(false);

    auto dst_b = ast::as<type::Bool>(arg1());

    if ( dst_b ) {
        setResult(true);
        return;
    }

    auto dst_i = ast::as<type::Integer>(arg1());

    if ( (i->signed_() && ! dst_i->signed_()) ||
         (! i->signed_() && dst_i->signed_()) ) {
        setResult(false);
        return;
    }

    if ( i->width() <= dst_i->width() ) {
        setResult(true);
        return;
    }
}
