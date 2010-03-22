// $Id$

#include "hilti.h"

hlt_stack* hlt_stack_new(int size, hlt_exception** excpt)
{
    if ( ! size )
        size = HLT_BOUND_FUNCTION_STACK_SIZE;
    
    hlt_stack* s = hlt_gc_malloc_non_atomic(sizeof(hlt_stack));
    
    if ( ! s ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    s->fptr = hlt_gc_malloc_non_atomic(size);
    if ( ! s->fptr ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }
     
    s->eoss = s->fptr + size;
    
    return s;
}
