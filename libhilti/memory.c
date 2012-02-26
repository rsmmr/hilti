
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "memory.h"
#include "rtti.h"
#include "debug.h"

#ifdef DEBUG
const char* __hlt_make_location(const char* file, int line)
{
    static char buffer[128];

    const char* s = strrchr(file, '/');
    if ( s )
        file = s + 1;

    snprintf(buffer, sizeof(buffer), "%s:%d", file, line);
    return buffer;
}
#endif

static void __dbg_mem(const char* tag, void* p, size_t size, const char* postfix, const char* location)
{
    if ( ! location )
        location = "";

    if ( size )
        DBG_LOG("hilti-mem", "%5s: %p (size %llu) %s %s", tag, p, size, postfix, location);
    else
        DBG_LOG("hilti-mem", "%5s: %p %s %s", tag, p, postfix, location);
}

static void __dbg_mem_obj(const char* tag, void* obj, const hlt_type_info* ti, size_t size, const char* postfix, int gc, const char* location)
{
    if ( ! location )
        location = "";

    if ( gc ) {
        __hlt_gchdr* hdr = (__hlt_gchdr*)obj;
        if ( size )
            DBG_LOG("hilti-mem", "%5s: %p (%s, ref %" PRIu64 ", size %llu) %s %s", tag, hdr, ti ? ti->tag : "<no rtti>", hdr ? hdr->ref_cnt : 0, size, postfix, location);
        else
            DBG_LOG("hilti-mem", "%5s: %p (%s, ref %" PRIu64 ") %s %s", tag, hdr, ti ? ti->tag : "<no rtti>", hdr ? hdr->ref_cnt : 0, postfix, location);
    }

    else {
        if ( size )
            DBG_LOG("hilti-mem", "%5s: %p (%s, no ref, size %llu) %s %s", tag, obj, ti ? ti->tag : "<no rtti>", size, postfix, location);
        else
            DBG_LOG("hilti-mem", "%5s: %p (%s, no ref) %s %s", tag, obj, ti ? ti->tag : "<no rtti>", postfix, location);
    }
}

void* __hlt_object_new(const hlt_type_info* ti, size_t size, const char* location)
{
    if ( ! location )
        location = "";

    __hlt_gchdr* hdr = (__hlt_gchdr*)hlt_calloc(1, size);
    hdr->ref_cnt = 1;

    __dbg_mem_obj("new", hdr, ti, size, "", ti->gc, location);

    return hdr;
}

void __hlt_object_ref(const hlt_type_info* ti, void* obj)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;

#ifdef DEBUG
    if ( ! ti->gc ) {
        __dbg_mem_obj("ref", hdr, ti, 0, "OBJECT NOT GCed", ti->gc, 0);
        abort();
    }

    if ( hdr->ref_cnt <= 0 ) {
        __dbg_mem_obj("ref", hdr, ti, 0, "BAD REF COUNT", ti->gc, 0);
        abort();
    }
#endif

    ++hdr->ref_cnt;

    __dbg_mem_obj("ref", hdr, ti, 0, "", ti->gc, 0);

}

void __hlt_object_unref(const hlt_type_info* ti, void* obj)
{
    if ( ! obj )
        return;

    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;

#ifdef DEBUG
    if ( ! ti->gc ) {
        __dbg_mem_obj("unref", hdr, ti, 0, "OBJECT NOT GCed", ti->gc, 0);
        abort();
    }

    if ( hdr->ref_cnt <= 0 ) {
        __dbg_mem_obj("unref", hdr, ti, 0, "BAD REF COUNT", ti->gc, 0);
        abort();
    }
#endif

    // We explicitly set it to zero even if we're about to delete the object.
    // That can catch some errors when the destroyed objects is still
    // accessed later.
    if ( --hdr->ref_cnt == 0 ) {
        __dbg_mem_obj("unref", hdr, ti, 0, "-> delete", ti->gc, 0);

        if ( ti->obj_dtor )
            (*(ti->obj_dtor))(ti, obj);

        hlt_free(obj);
    }

    else
        __dbg_mem_obj("unref", hdr, ti, 0, "", ti->gc, 0);
}

void __hlt_object_dtor(const hlt_type_info* ti, void* obj, const char* location)
{
    typedef void (*fptr)(void *obj);

    if ( ti && ti->dtor ) {
        if ( ti->gc ) {
            __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

            if ( ! hdr )
                return;

            __dbg_mem_obj("dtor", hdr, ti, 0, "", ti->gc, location);
            (*(ti->dtor))(ti, *(void**)obj);
        }
        else {
            __dbg_mem_obj("dtor", obj, ti, 0, "", ti->gc, location);
            (*(ti->dtor))(ti, obj);
        }
    }
}

void __hlt_object_cctor(const hlt_type_info* ti, void* obj, const char* location)
{
    typedef void (*fptr)(void *obj);

    if ( ti && ti->cctor ) {
        if ( ti->gc ) {
            __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

            if ( ! hdr )
                return;

            __dbg_mem_obj("cctor", hdr, ti, 0, "", ti->gc, location);
            (*(ti->cctor))(ti, *(void**)obj);
        }
        else {
            __dbg_mem_obj("cctor", obj, ti, 0, "", ti->gc, location);
            (*(ti->cctor))(ti, obj);
        }
    }
}

void* hlt_malloc(size_t size)
{
    void *p = malloc(size);

    __dbg_mem("malloc", p, size, "", 0);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

    return p;
}

void* hlt_calloc(size_t count, size_t size)
{
    void *p = calloc(count, size);

    __dbg_mem("calloc", p, size, "", 0);

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

    __dbg_mem("free", memory, 0, "", 0);

    free(memory);
}
