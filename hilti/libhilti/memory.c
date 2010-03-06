/* $Id 
 * 
 * GC-based replaced functions for memory management.
 * 
 * Todo: Well, currently these are actually *not* garbage collected ... 
 * 
 */

#include "hilti.h"

void* hlt_gc_malloc_atomic(uint64_t n)
{
    return malloc(n);
}

void* hlt_gc_malloc_non_atomic(uint64_t n)
{
    return malloc(n);
}

void* hlt_gc_calloc_atomic(uint64_t count, uint64_t n)
{
    return calloc(count, n);
}

void* hlt_gc_calloc_non_atomic(uint64_t count, uint64_t n)
{
    return calloc(count, n);
}

void* hlt_gc_realloc_atomic(void* ptr, uint64_t n)
{
    return realloc(ptr, n);
}

void* hlt_gc_realloc_non_atomic(void* ptr, uint64_t n)
{
    return realloc(ptr, n);
}

