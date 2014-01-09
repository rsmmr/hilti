
#include <stdio.h>

#include "callable.h"
#include "context.h"

void hlt_callable_dtor(hlt_type_info* ti, hlt_callable* c)
{
    if ( c->__func->dtor )
        (*c->__func->dtor)(c);
}
