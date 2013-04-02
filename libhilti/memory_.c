
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "memory_.h"
#include "globals.h"
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

static void _dbg_mem_raw(const char* op, void* obj, uint64_t size, const char* type, const char* location, const char* aux)
{
    static char buf[128];

    assert(obj);

    if ( ! (location && *location) )
        location = "-";

    if ( aux )
        snprintf(buf, sizeof(buf), " (%s)", aux);
    else
        buf[0] = '\0';

    DBG_LOG("hilti-mem", "%10s %p %" PRIu64 " %" PRIu64 " %s %s%s", op, obj, size, 0, type, location, buf);
}

static void _dbg_mem_gc(const char* op, const hlt_type_info* ti, void* gcobj, const char* location, const char* aux)
{
    static char buf[128];

    assert(gcobj);
    assert(ti);

    if ( ! (location && *location) )
        location = "-";

    if ( aux )
        snprintf(buf, sizeof(buf), " (%s)", aux);
    else
        buf[0] = '\0';

    __hlt_gchdr* hdr = (__hlt_gchdr*)gcobj;
    DBG_LOG("hilti-mem", "%10s %p %" PRIu64 " %" PRIu64 " %s %s%s", op, gcobj, 0, hdr->ref_cnt, ti->tag, location, buf);
}

static void _internal_memory_error(void* gcobj, const char* func, const char* msg)
{
    fprintf(stderr, "internal memory error in %s for object %p: %s\n", func, gcobj, msg);
    abort();
}

#endif

void* __hlt_malloc(uint64_t size, const char* type, const char* location)
{
    void *p = calloc(1, size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc", p, size, type, location, 0);
#endif

    return p;
}

void* __hlt_realloc(void* p, uint64_t size, const char* type, const char* location)
{
#ifdef DEBUG
    if ( p ) {
        ++__hlt_globals()->num_deallocs;
        _dbg_mem_raw("free", p, size, type, location, "realloc");
    }
#endif

    p = realloc(p, size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc", p, size, type, location, "realloc");
#endif

    return p;
}

void* __hlt_calloc(uint64_t count, uint64_t size, const char* type, const char* location)
{
    void *p = calloc(count, size);

    if ( ! p ) {
        fputs("out of memory in hlt_calloc, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("calloc", p, count * size, type, location, 0);
#endif

    return p;
}

void __hlt_free(void *memory, const char* type, const char* location)
{
    if ( ! memory )
        return;

#ifdef DEBUG
    ++__hlt_globals()->num_deallocs;
    _dbg_mem_raw("free", memory, 0, type, location, 0);
#endif

    free(memory);
}


void* __hlt_object_new(const hlt_type_info* ti, uint64_t size, const char* location)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)__hlt_malloc(size, ti->tag, location);
    hdr->ref_cnt = 1;

#ifdef DEBUG
    _dbg_mem_gc("new", ti, hdr, location, 0);
#endif

    return hdr;
}

void __hlt_object_ref(const hlt_type_info* ti, void* obj)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;

#ifdef DEBUG
    if ( ! ti->gc ) {
        _dbg_mem_gc("! ref", ti, obj, 0, 0);
        _internal_memory_error(obj, "__hlt_object_ref", "object not garbage collected");
    }

    if ( hdr->ref_cnt <= 0 ) {
        _dbg_mem_gc("! ref", ti, obj, 0, 0);
        _internal_memory_error(obj, "__hlt_object_ref", "bad reference count");
    }
#endif

    ++hdr->ref_cnt;

#ifdef DEBUG
    ++__hlt_globals()->num_refs;
    _dbg_mem_gc("ref", ti, obj, 0, 0);
#endif

}

void __hlt_object_unref(const hlt_type_info* ti, void* obj)
{
    if ( ! obj )
        return;

    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;

#ifdef DEBUG
    const char* aux = 0;

    if ( hdr->ref_cnt == 0 )
        aux = "dtor";

    if ( ! ti->gc ) {
        _dbg_mem_gc("! unref", ti, obj, 0, aux);
        _internal_memory_error(obj, "__hlt_object_unref", "object not garbage collected");
    }

    if ( hdr->ref_cnt <= 0 ) {
        _dbg_mem_gc("! unref", ti, obj, 0, aux);
        _internal_memory_error(obj, "__hlt_object_unref", "bad reference count");
    }
#endif

    // We explicitly set it to zero even if we're about to delete the object.
    // That can catch some errors when the destroyed objects is still
    // accessed later.
    --hdr->ref_cnt;

#ifdef DEBUG
    ++__hlt_globals()->num_unrefs;
    _dbg_mem_gc("unref", ti, obj, 0, aux);
#endif

    if ( hdr->ref_cnt == 0 ) {
        if ( ti->obj_dtor )
            (*(ti->obj_dtor))(ti, obj);

        __hlt_free(obj, ti->tag, "unref");
    }
}

void __hlt_object_dtor(const hlt_type_info* ti, void* obj, const char* location)
{
    typedef void (*fptr)(void *obj);

    if ( ! obj )
        return;

    if ( ! (ti && ti->dtor) )
        return;

    if ( ti->gc ) {
        __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

        if ( ! hdr )
            return;

#ifdef DEBUG
        _dbg_mem_gc("dtor", ti, *(void**)obj, location, 0);
#endif

        (*(ti->dtor))(ti, *(void**)obj);
    }

    else {
#ifdef DEBUG
        _dbg_mem_raw("dtor", obj, 0, ti->tag, location, 0);
#endif
        (*(ti->dtor))(ti, obj);
    }
}

void __hlt_object_cctor(const hlt_type_info* ti, void* obj, const char* location)
{
    typedef void (*fptr)(void *obj);

    if ( ! obj )
        return;

    if ( ! (ti && ti->cctor) )
        return;

    if ( ti->gc ) {
        __hlt_gchdr* hdr = *(__hlt_gchdr**)obj;

        if ( ! hdr )
            return;

#ifdef DEBUG
        _dbg_mem_gc("cctor", ti, *(void**)obj, location, 0);
#endif

        (*(ti->cctor))(ti, *(void**)obj);
    }

    else {
#ifdef DEBUG
        _dbg_mem_raw("cctor", obj, 0, ti->tag, location, 0);
#endif
        (*(ti->cctor))(ti, obj);
    }
}

hlt_free_list* hlt_free_list_new()
{
    hlt_free_list* list = hlt_malloc(sizeof(hlt_free_list));
    list->pool = 0;
    list->size = 0;
    return list;
}

void* hlt_free_list_alloc(hlt_free_list* list, size_t size)
{
    assert(size == list->size || ! list->size);

    __hlt_free_list_block* b = 0;

    if ( list->pool ) {
        b = list->pool;
        list->pool = list->pool->next;
        bzero(b, sizeof(__hlt_free_list_block) + size); // Make contents consistent.
    }
    else {
        b = (__hlt_free_list_block*) hlt_malloc(sizeof(__hlt_free_list_block) + size);
        list->size = size;
    }

    return ((char *)b) + sizeof(__hlt_free_list_block*);
}

void hlt_free_list_free(hlt_free_list* list, void* p, size_t size)
{
    assert(size == list->size);

    __hlt_free_list_block* b = (__hlt_free_list_block*) (((char *)p) - sizeof(__hlt_free_list_block*));
    b->next = list->pool;
    list->pool = b;
}

void hlt_free_list_delete(hlt_free_list* list)
{
    __hlt_free_list_block* b = list->pool;

    while ( b ) {
        __hlt_free_list_block* tmp = b->next;
        hlt_free(b);
        b = tmp;
    }

    hlt_free(list);
}

hlt_memory_stats hlt_memory_statistics()
{
    hlt_memory_stats stats;
    hlt_memory_usage(&stats.size_heap, &stats.size_alloced);

    __hlt_global_state* globals = __hlt_globals();
    stats.num_allocs = globals->num_allocs;
    stats.num_deallocs = globals->num_deallocs;
    stats.num_refs = globals->num_refs;
    stats.num_unrefs = globals->num_unrefs;

    return stats;
}
