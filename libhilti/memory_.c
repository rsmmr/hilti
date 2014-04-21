
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "memory_.h"
#include "globals.h"
#include "rtti.h"
#include "debug.h"

#ifndef HLT_DEEP_COPY_VALUES_ACROSS_THREADS
#define HLT_ATOMIC_REF_COUNTING
#endif

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

static void _internal_memory_error(void* gcobj, const char* func, const char* msg, const hlt_type_info* ti)
{
    const char* type = ti ? ti->tag : "<unknown>";
    fprintf(stderr, "internal memory error in %s for object %p of type %s: %s\n", func, gcobj, type, msg);
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

void* __hlt_realloc(void* p, uint64_t size, uint64_t old_size, const char* type, const char* location)
{
#ifdef DEBUG
    if ( p ) {
        ++__hlt_globals()->num_deallocs;
        _dbg_mem_raw("free", p, size, type, location, "realloc");
    }
#endif

    if ( size > old_size ) {
        p = realloc(p, size);
        memset(p + old_size, 0, size - old_size);

        if ( ! p ) {
            fputs("out of memory in hlt_malloc, aborting", stderr);
            exit(1);
        }
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc", p, size, type, location, "realloc");
#endif

    return p;
}

void* __hlt_realloc_no_init(void* p, uint64_t size, const char* type, const char* location)
{
#ifdef DEBUG
    if ( p ) {
        ++__hlt_globals()->num_deallocs;
        _dbg_mem_raw("free", p, size, type, location, "realloc_no_init");
    }
#endif

    p = realloc(p, size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc", p, size, type, location, "realloc_no_init");
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
    assert(size);

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
        _internal_memory_error(obj, "__hlt_object_ref", "object not garbage collected", ti);
    }
#endif

#ifdef DEBUG
    int64_t new_ref_cnt =
#endif

#ifdef HLT_ATOMIC_REF_COUNTING
    __atomic_add_fetch(&hdr->ref_cnt, 1, __ATOMIC_SEQ_CST);
#else
    ++hdr->ref_cnt;
#endif

#ifdef DEBUG
    if ( new_ref_cnt <= 0 ) {
        _dbg_mem_gc("! ref", ti, obj, 0, 0);
        _internal_memory_error(obj, "__hlt_object_ref", "bad reference count", ti);
    }
#endif

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
    if ( ! ti->gc ) {
        _dbg_mem_gc("! unref", ti, obj, 0, "");
        _internal_memory_error(obj, "__hlt_object_unref", "object not garbage collected", ti);
    }
#endif

#ifdef HLT_ATOMIC_REF_COUNTING
    int64_t new_ref_cnt = __atomic_sub_fetch(&hdr->ref_cnt, 1, __ATOMIC_SEQ_CST);
#else
    int64_t new_ref_cnt = --hdr->ref_cnt;
#endif

#ifdef DEBUG
    const char* aux = 0;

    if ( new_ref_cnt == 0 )
        aux = "dtor";

    if ( new_ref_cnt < 0 ) {
        _dbg_mem_gc("! unref", ti, obj, 0, aux);
        _internal_memory_error(obj, "__hlt_object_unref", "bad reference count", ti);
    }
#endif

#ifdef DEBUG
    ++__hlt_globals()->num_unrefs;
    _dbg_mem_gc("unref", ti, obj, 0, aux);
#endif

    if ( new_ref_cnt == 0 ) {
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

void* hlt_stack_alloc(size_t size)
{
#ifndef DARWIN
    void* stack = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
#else
    void* stack = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_GROWSDOWN | MAP_NORESERVE, -1, 0);
#endif

    if ( stack == MAP_FAILED ) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_stacks;
    __hlt_globals()->size_stacks += size;
#endif

    return stack;
}

void hlt_stack_invalidate(void* stack, size_t size)
{
    if ( msync(stack, size, MS_INVALIDATE) < 0 ) {
        fprintf(stderr, "msync failed: %s\n", strerror(errno));
        exit(1);
    }
}

void hlt_stack_free(void* stack, size_t size)
{
    if ( munmap(stack, size) < 0 ) {
        fprintf(stderr, "munmap failed: %s\n", strerror(errno));
        exit(1);
    }

#ifdef DEBUG
    --__hlt_globals()->num_stacks;
    __hlt_globals()->size_stacks -= size;
#endif
}

hlt_free_list* hlt_free_list_new(size_t size)
{
    hlt_free_list* list = hlt_malloc(sizeof(hlt_free_list));
    list->pool = 0;
    list->size = sizeof(__hlt_free_list_block) + size;
    return list;
}


size_t hlt_free_list_block_size(hlt_free_list* list)
{
    return list->size - sizeof(__hlt_free_list_block);
}

#define __data_offset ((int) &((__hlt_free_list_block*)0)->data)

void* hlt_free_list_alloc(hlt_free_list* list)
{
    __hlt_free_list_block* b = 0;

    if ( list->pool ) {
        b = list->pool;
        list->pool = list->pool->next;
        bzero(b, list->size); // Make contents consistent.
    }
    else {
        b = (__hlt_free_list_block*) hlt_malloc(list->size);
    }

    return ((char *)b) + __data_offset;
}

void hlt_free_list_free(hlt_free_list* list, void* p)
{
    __hlt_free_list_block* b = (__hlt_free_list_block*) (((char *)p) - __data_offset);

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
    stats.size_stacks = globals->size_stacks;
    stats.num_stacks = globals->num_stacks;

    return stats;
}
