
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include "memory.h"
#include "rtti.h"
#include "debug.h"

static void __dbg_mem(const char* tag, void* p, size_t size, const char* postfix)
{
    if ( size )
        DBG_LOG("hilti-mem", "%5s: %p (size %llu) %s", tag, p, size, postfix);
    else
        DBG_LOG("hilti-mem", "%5s: %p %s", tag, p, postfix);
}

static void __dbg_mem_obj(const char* tag, void* obj, const hlt_type_info* ti, size_t size, const char* postfix, int gc)
{
    if ( gc ) {
        __hlt_gchdr* hdr = (__hlt_gchdr*)obj;
        if ( size )
            DBG_LOG("hilti-mem", "%5s: %p (%s, ref %" PRIu64 ", size %llu) %s", tag, hdr, ti ? ti->tag : "<no rtti>", hdr->ref_cnt, size, postfix);
        else
            DBG_LOG("hilti-mem", "%5s: %p (%s, ref %" PRIu64 ") %s", tag, hdr, ti ? ti->tag : "<no rtti>", hdr->ref_cnt, postfix);
    }

    else {
        if ( size )
            DBG_LOG("hilti-mem", "%5s: %p (%s, no ref, size %llu) %s", tag, obj, ti ? ti->tag : "<no rtti>", size, postfix);
        else
            DBG_LOG("hilti-mem", "%5s: %p (%s, no ref) %s", tag, obj, ti ? ti->tag : "<no rtti>", postfix);
    }
}

void* __hlt_object_new(const hlt_type_info* ti, size_t size)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)hlt_calloc(1, size);
    hdr->ref_cnt = 1;

    __dbg_mem_obj("new", hdr, ti, size, "", ti->gc);

    return hdr;
}

void __hlt_object_ref(const hlt_type_info* ti, void* obj)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;
    assert(hdr->ref_cnt > 0);
    ++hdr->ref_cnt;

    __dbg_mem_obj("ref", hdr, ti, 0, "", ti->gc);
}

void __hlt_object_unref(const hlt_type_info* ti, void* obj)
{
    if ( ! obj )
        return;

    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;
    assert(hdr->ref_cnt > 0);

    // We explicitly set it to zero even if we're about to delete the object.
    // That can catch some errors when the destroyed objects is still
    // accessed later.
    if ( --hdr->ref_cnt == 0 ) {
        __dbg_mem_obj("unref", hdr, ti, 0, "-> delete", ti->gc);

        if ( ti->obj_dtor )
            (*(ti->obj_dtor))(ti, obj);

        hlt_free(obj);
    }

    else
        __dbg_mem_obj("unref", hdr, ti, 0, "", ti->gc);
}

void __hlt_object_dtor(const hlt_type_info* ti, void* obj)
{
    typedef void (*fptr)(void *obj);

    if ( ti && ti->dtor ) {
        if ( ti->gc ) {
            __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

            if ( ! hdr )
                return;

            __dbg_mem_obj("dtor", hdr, ti, 0, "", ti->gc);
            (*(ti->dtor))(ti, *(void**)obj);
        }
        else {
            __dbg_mem_obj("dtor", obj, ti, 0, "", ti->gc);
            (*(ti->dtor))(ti, obj);
        }
    }
}

void __hlt_object_cctor(const hlt_type_info* ti, void* obj)
{
    typedef void (*fptr)(void *obj);

    if ( ti && ti->cctor ) {
        if ( ti->gc ) {
            __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

            if ( ! hdr )
                return;

            __dbg_mem_obj("cctor", hdr, ti, 0, "", ti->gc);
            (*(ti->cctor))(ti, *(void**)obj);
        }
        else {
            __dbg_mem_obj("cctor", obj, ti, 0, "", ti->gc);
            (*(ti->cctor))(ti, obj);
        }
    }
}

void* hlt_malloc(size_t size)
{
    void *p = malloc(size);

    __dbg_mem("malloc", p, size, "");

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

    return p;
}

void* hlt_calloc(size_t count, size_t size)
{
    void *p = calloc(count, size);

    __dbg_mem("calloc", p, size, "");

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

    __dbg_mem("free", memory, 0, "");

    free(memory);
}
