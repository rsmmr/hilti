
#include <stdio.h>
#include <assert.h>

#include "memory.h"
#include "rtti.h"

void* hlt_object_new(const hlt_type_info* ti, size_t size)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)hlt_malloc(size);
    hdr->ref_cnt = 1;
    return hdr;
}

void hlt_object_ref(const hlt_type_info* ti, void* obj)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;
    assert(hdr->ref_cnt > 0);
    ++hdr->ref_cnt;
}

void hlt_object_unref(const hlt_type_info* ti, void* obj)
{
    if ( ! obj )
        return;

    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;
    assert(hdr->ref_cnt > 0);

    // We explicitly set it to zero even if we're about to delete the object.
    // That can catch some errors when the destroyed objects is still
    // accessed later.
    if ( --hdr->ref_cnt == 0 ) {
        typedef void (*fptr)(void *obj);
        (*(ti->dtor))(obj);
        hlt_free(obj);
    }
}

void* hlt_malloc(size_t size)
{
    void *p = malloc(size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

    return p;
}

void* hlt_calloc(size_t count, size_t size)
{
    void *p = calloc(count, size);

    if ( ! p ) {
        fputs("out of memory in hlt_calloc, aborting", stderr);
        exit(1);
    }

    return p;
}

void hlt_free(void *memory)
{
    if ( ! memory )
        return;

    free(memory);
}
