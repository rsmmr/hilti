/* $Id 
 * 
 * GC-based replaced functions for memory management.
 * 
 * Todo: Well, currently these are actually *not* garbage collected ... 
 * 
 */

#include "hilti.h"

void* hlt_gc_malloc_atomic(size_t n)
{
    return malloc(n);
}

void* hlt_gc_malloc_non_atomic(size_t n)
{
    return malloc(n);
}

void* hlt_gc_calloc_atomic(size_t count, size_t n)
{
    return calloc(count, n);
}

void* hlt_gc_calloc_non_atomic(size_t count, size_t n)
{
    return calloc(count, n);
}

void* hlt_gc_realloc_atomic(void* ptr, size_t n)
{
    return realloc(ptr, n);
}

void* hlt_gc_realloc_non_atomic(void* ptr, size_t n)
{
    return realloc(ptr, n);
}

