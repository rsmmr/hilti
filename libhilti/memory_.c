
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
#include "context.h"

#ifndef HLT_DEEP_COPY_VALUES_ACROSS_THREADS
#define HLT_ATOMIC_REF_COUNTING
#endif

static const size_t __INITIAL_NULLBUFFER_SIZE = 20;

struct __obj_with_rtti {
    const hlt_type_info* ti;
    void* obj;
};

struct __hlt_memory_nullbuffer {
    size_t used;
    size_t allocated;
    int64_t flush_pos;
    struct __obj_with_rtti* objs;
};

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

static void _dbg_mem_raw(const char* op, void* obj, uint64_t size, const char* type, const char* location, const char* aux, hlt_execution_context* ctx)
{
    static char buf[128];

    if ( ! (location && *location) )
        location = "-";

    if ( aux )
        snprintf(buf, sizeof(buf), " (%s)", aux);
    else
        buf[0] = '\0';

    const char* nb = (ctx && obj && __hlt_memory_nullbuffer_contains(ctx->nullbuffer, obj)) ? " (in nullbuffer)" : "";

    DBG_LOG("hilti-mem", "%10s %p %" PRIu64 " %" PRIu64 " %s %s%s%s", op, obj, size, 0, type, location, nb, buf);
}

static void _dbg_mem_gc(const char* op, const hlt_type_info* ti, void* gcobj, const char* location, const char* aux, hlt_execution_context* ctx)
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

    const char* nb = (ctx && gcobj && __hlt_memory_nullbuffer_contains(ctx->nullbuffer, gcobj)) ? " (in nullbuffer)" : "";

    __hlt_gchdr* hdr = (__hlt_gchdr*)gcobj;
    DBG_LOG("hilti-mem", "%10s %p %" PRIu64 " %" PRIu64 " %s %s%s%s", op, gcobj, 0, hdr->ref_cnt, ti->tag, location, nb, buf);
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
    _dbg_mem_raw("malloc", p, size, type, location, 0, 0);
#endif

    return p;
}

void* __hlt_malloc_no_init(uint64_t size, const char* type, const char* location)
{
    void *p = malloc(size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc_no_init, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc_no_init", p, size, type, location, 0, 0);
#endif

    return p;
}

void* __hlt_realloc(void* p, uint64_t size, uint64_t old_size, const char* type, const char* location)
{
#ifdef DEBUG
    if ( p ) {
        ++__hlt_globals()->num_deallocs;
        _dbg_mem_raw("free", p, size, type, location, "realloc", 0);
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
    _dbg_mem_raw("malloc", p, size, type, location, "realloc", 0);
#endif

    return p;
}

void* __hlt_realloc_no_init(void* p, uint64_t size, const char* type, const char* location)
{
#ifdef DEBUG
    if ( p ) {
        ++__hlt_globals()->num_deallocs;
        _dbg_mem_raw("free", p, size, type, location, "realloc_no_init", 0);
    }
#endif

    p = realloc(p, size);

    if ( ! p ) {
        fputs("out of memory in hlt_malloc, aborting", stderr);
        exit(1);
    }

#ifdef DEBUG
    ++__hlt_globals()->num_allocs;
    _dbg_mem_raw("malloc", p, size, type, location, "realloc_no_init", 0);
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
    _dbg_mem_raw("calloc", p, count * size, type, location, 0, 0);
#endif

    return p;
}

void __hlt_free(void *memory, const char* type, const char* location)
{
    if ( ! memory )
        return;

#ifdef DEBUG
    ++__hlt_globals()->num_deallocs;
    _dbg_mem_raw("free", memory, 0, type, location, 0, 0);
#endif

    free(memory);
}


void* __hlt_object_new_ref(const hlt_type_info* ti, uint64_t size, const char* location, hlt_execution_context* ctx)
{
    assert(size);

    __hlt_gchdr* hdr = (__hlt_gchdr*)__hlt_malloc(size, ti->tag, location);
    hdr->ref_cnt = 1;

#ifdef DEBUG
    _dbg_mem_gc("new_ref", ti, hdr, location, 0, ctx);
#endif

    return hdr;
}

void* __hlt_object_new(const hlt_type_info* ti, uint64_t size, const char* location, hlt_execution_context* ctx)
{
    assert(size);
    assert(ctx->nullbuffer->flush_pos < 0);

    __hlt_gchdr* hdr = (__hlt_gchdr*)__hlt_malloc(size, ti->tag, location);
    hdr->ref_cnt = 0;
    __hlt_memory_nullbuffer_add(ctx->nullbuffer, ti, hdr, ctx);

#ifdef DEBUG
    _dbg_mem_gc("new", ti, hdr, location, 0, ctx);
#endif

    return hdr;
}

void* __hlt_object_new_no_init_ref(const hlt_type_info* ti, uint64_t size, const char* location, hlt_execution_context* ctx)
{
    assert(size);

    __hlt_gchdr* hdr = (__hlt_gchdr*)__hlt_malloc_no_init(size, ti->tag, location);
    hdr->ref_cnt = 1;

#ifdef DEBUG
    _dbg_mem_gc("new_ref", ti, hdr, location, 0, ctx);
#endif

    return hdr;
}

void* __hlt_object_new_no_init(const hlt_type_info* ti, uint64_t size, const char* location, hlt_execution_context* ctx)
{
    assert(size);
    assert(ctx->nullbuffer->flush_pos < 0);

    __hlt_gchdr* hdr = (__hlt_gchdr*)__hlt_malloc_no_init(size, ti->tag, location);
    hdr->ref_cnt = 0;
    __hlt_memory_nullbuffer_add(ctx->nullbuffer, ti, hdr, ctx);

#ifdef DEBUG
    _dbg_mem_gc("new", ti, hdr, location, 0, ctx);
#endif

    return hdr;
}

void __hlt_object_ref(const hlt_type_info* ti, void* obj, hlt_execution_context* ctx)
{
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;;

#ifdef DEBUG
    if ( ! ti->gc ) {
        _dbg_mem_gc("! ref", ti, obj, 0, 0, ctx);
        _internal_memory_error(obj, "__hlt_object_ref", "object not garbage collected", ti);
    }
#endif

#ifdef HLT_ATOMIC_REF_COUNTING
    __atomic_add_fetch(&hdr->ref_cnt, 1, __ATOMIC_SEQ_CST);
#else
    ++hdr->ref_cnt;
#endif

#if 0
    // This is ok now!
    if ( new_ref_cnt <= 0 ) {
        _dbg_mem_gc("! ref", ti, obj, 0, 0, ctx);
        _internal_memory_error(obj, "__hlt_object_ref", "bad reference count", ti);
    }
#endif

#ifdef DEBUG
    ++__hlt_globals()->num_refs;
    _dbg_mem_gc("ref", ti, obj, 0, 0, ctx);
#endif

}

void __hlt_object_unref(const hlt_type_info* ti, void* obj, hlt_execution_context* ctx)
{
    if ( ! obj )
        return;

    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;

#ifdef DEBUG
    if ( ! ti->gc ) {
        _dbg_mem_gc("! unref", ti, obj, 0, "", ctx);
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

#if 0
    // This is now ok !
    if ( new_ref_cnt < 0 ) {
        _dbg_mem_gc("! unref", ti, obj, 0, aux, ctx);
        _internal_memory_error(obj, "__hlt_object_unref", "bad reference count", ti);
    }
#endif
#endif

#ifdef DEBUG
    ++__hlt_globals()->num_unrefs;
    _dbg_mem_gc("unref", ti, obj, 0, aux, ctx);
#endif

    if ( new_ref_cnt == 0 )
        __hlt_memory_nullbuffer_add(ctx->nullbuffer, ti, hdr, ctx);
}

void __hlt_object_destroy(const hlt_type_info* ti, void* obj, const char* location, hlt_execution_context* ctx)
{
    assert(ti->gc);

    if ( ! ti->obj_dtor )
        return;

#ifdef DEBUG
    _dbg_mem_gc("destroy", ti, obj, location, 0, ctx);
#endif

    (*(ti->obj_dtor))(ti, obj, ctx);
}

void __hlt_object_dtor(const hlt_type_info* ti, void* obj, const char* location, hlt_execution_context* ctx)
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
        _dbg_mem_gc("dtor", ti, *(void**)obj, location, 0, ctx);
#endif

        (*(ti->dtor))(ti, *(void**)obj, ctx);
    }

    else {
#ifdef DEBUG
        _dbg_mem_raw("dtor", obj, 0, ti->tag, location, 0, ctx);
#endif
        (*(ti->dtor))(ti, obj, ctx);
    }
}

void __hlt_object_cctor(const hlt_type_info* ti, void* obj, const char* location, hlt_execution_context* ctx)
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
        _dbg_mem_gc("cctor", ti, *(void**)obj, location, 0, ctx);
#endif

        (*(ti->cctor))(ti, *(void**)obj, ctx);
    }

    else {
#ifdef DEBUG
        _dbg_mem_raw("cctor", obj, 0, ti->tag, location, 0, ctx);
#endif
        (*(ti->cctor))(ti, obj, ctx);
    }
}

#define UNW_LOCAL_ONLY
#include <libunwind.h>

static void _adjust_stack_pre_safepoint(hlt_execution_context* ctx)
{
    // Climb up the stack and adjust the reference counts for live objects
    // referenced there so that it reflects reality.

    unw_cursor_t cursor;
    unw_context_t uc;
    unw_word_t ip, sp;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);

    int level = 0;

    while ( unw_step(&cursor) > 0 ) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        printf ("level = %d ip = %lx, sp = %lx\n", level, (long) ip, (long) sp);
        level++;
    }

#ifdef DEBUG
    // DBG_LOG("hilti-mem", "%10s stack-adjust-ctor %p %d %s::%s = %s", obj, level, func, id, render);
#endif
}

void __hlt_memory_safepoint(hlt_execution_context* ctx, const char* location)
{
#ifdef DEBUG
    _dbg_mem_raw("safepoint", 0, 0, 0, location, 0, ctx);
#endif

    // Work in progress.
    // _adjust_stack_pre_safepoint(ctx);
    __hlt_memory_nullbuffer_flush(ctx->nullbuffer, ctx);
}

void* hlt_stack_alloc(size_t size)
{
#ifdef DARWIN
    void* stack = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
    void* stack = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN | MAP_NORESERVE, -1, 0);
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
    else
        b = (__hlt_free_list_block*) hlt_malloc(list->size);

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

void hlt_memory_pool_init(hlt_memory_pool* p, size_t size)
{
    p->last = &p->first;

    p->first.next = 0;
    p->first.cur = &p->first.data[0];
    p->first.end = &p->first.data[0] + size;
}

void hlt_memory_pool_dtor(hlt_memory_pool* p)
{
    __hlt_memory_pool_block* b;
    __hlt_memory_pool_block* n;

    for ( b = p->first.next; b; b = n ) { // Don't delete first.
        n = b->next;
        hlt_free(b);
    }
}

void* hlt_memory_pool_malloc(hlt_memory_pool* p, size_t n)
{
    return hlt_memory_pool_calloc(p, 1, n);
}

void* hlt_memory_pool_calloc(hlt_memory_pool* p, size_t count, size_t n)
{
    size_t avail = p->last->end - p->last->cur;

#ifdef DEBUG
    if ( p->last->cur == &p->first.data[0] && n > avail )
        fprintf(stderr, "libhilti debug warning: memory pool block size cannot even fit first alloc");
#endif

    if ( n > avail ) {
        // Need to alloc a new block.
        size_t dsize = p->first.end - &p->first.data[0];
        size_t bsize = n < dsize ? n : dsize;
        __hlt_memory_pool_block* b = hlt_calloc(1, sizeof(__hlt_memory_pool_block) + bsize);

        b->cur = &b->data[0];
        b->end = &b->data[0] + bsize;
        b->next = 0;

        p->last->next = b;
        p->last = b;
    }

    // Bump pointer to allocate.
    void* ptr = p->last->cur;
    p->last->cur += n;
    return ptr;
}

void  hlt_memory_pool_free(hlt_memory_pool* p, void* b)
{
    // Do nothing.
}

__hlt_memory_nullbuffer* __hlt_memory_nullbuffer_new()
{
    __hlt_memory_nullbuffer* nbuf = (__hlt_memory_nullbuffer*) hlt_malloc(sizeof(__hlt_memory_nullbuffer));
    nbuf->used = 0;
    nbuf->allocated = __INITIAL_NULLBUFFER_SIZE;
    nbuf->flush_pos = -1;
    nbuf->objs = (struct __obj_with_rtti*) hlt_malloc(sizeof(struct __obj_with_rtti) * nbuf->allocated);
    return nbuf;
}

void __hlt_memory_nullbuffer_delete(__hlt_memory_nullbuffer* nbuf, hlt_execution_context* ctx)
{
    __hlt_memory_nullbuffer_flush(nbuf, ctx);
    hlt_free(nbuf->objs);
    hlt_free(nbuf);
}

static inline int64_t _nullbuffer_index(__hlt_memory_nullbuffer* nbuf, void *obj)
{
    // TODO: Unclear if it's worth keeping the buffer sorted to speed this up?
    for ( int64_t i = 0; i < nbuf->used; i++ ) {
        if ( nbuf->objs[i].obj == obj )
            return i;
    }

    return -1;
}

void __hlt_memory_nullbuffer_add(__hlt_memory_nullbuffer* nbuf, const hlt_type_info* ti, void *obj, hlt_execution_context* ctx)
{
    int64_t nbpos = _nullbuffer_index(nbuf, obj);

    if ( nbuf->flush_pos >= 0 ) {
        // We're flushing.

        if ( nbpos == nbuf->flush_pos )
            // Deleting this right now.
            return;

        if ( nbpos > nbuf->flush_pos )
            // In the buffer, and still getting there.
            return;

        // Either not yet in the buffer, or we passed it already. Delete
        // directly, we must not further modify the nullbuffer while flushing.

#ifdef DEBUG
        __hlt_gchdr* hdr = (__hlt_gchdr*)obj;
        assert(hdr->ref_cnt <= 0);
#endif

        if ( ti->obj_dtor )
            (*(ti->obj_dtor))(ti, obj, ctx);

        if ( nbpos >= 0 )
            // Just to be safe.
            nbuf->objs[nbpos].obj = 0;

        __hlt_free(obj, ti->tag, "nullbuffer_add (during flush)");
        return;
    }

    if ( nbpos >= 0 )
        // Already in the buffer.
        return;

#ifdef DEBUG
    __hlt_gchdr* hdr = (__hlt_gchdr*)obj;
    _dbg_mem_gc("nullbuffer_add", ti, hdr, "", 0, ctx);
#endif

    if ( nbuf->used >= nbuf->allocated ) {
        size_t nsize = (nbuf->allocated * 2);
        nbuf->objs = (struct __obj_with_rtti*) hlt_realloc(nbuf->objs,
                                                           sizeof(struct __obj_with_rtti) * nsize,
                                                           sizeof(struct __obj_with_rtti) * nbuf->allocated);
        nbuf->allocated = nsize;
    }

    struct __obj_with_rtti x;
    x.ti = ti;
    x.obj = obj;
    nbuf->objs[nbuf->used++] = x;

#ifdef DEBUG
    ++__hlt_globals()->num_nullbuffer;

    if ( nbuf->used > __hlt_globals()->max_nullbuffer )
        // Not thread-safe, but doesn't matter.
        __hlt_globals()->max_nullbuffer = nbuf->used;
#endif
}

int8_t __hlt_memory_nullbuffer_contains(__hlt_memory_nullbuffer* nbuf, void *obj)
{
    return _nullbuffer_index(nbuf, obj) >= 0;
}

void __hlt_memory_nullbuffer_remove(__hlt_memory_nullbuffer* nbuf, void *obj)
{
    // TODO: Unclear if it's worth keeping the buffer sorted to speed this up?
    for ( size_t i = 0; i < nbuf->used; i++ ) {
        struct __obj_with_rtti x = nbuf->objs[i];

        if ( x.obj == obj ) {
            // Mark as done.
            x.obj = 0;
            --__hlt_globals()->num_nullbuffer;
            break;
        }
    }
}

void __hlt_memory_nullbuffer_flush(__hlt_memory_nullbuffer* nbuf, hlt_execution_context* ctx)
{
    if ( nbuf->flush_pos >= 0 )
        return;

#ifdef DEBUG
    _dbg_mem_raw("nullbuffer_flush", nbuf, nbuf->used, 0, "start", 0, ctx);
#endif

    // Note, flush_pos is examined during flushing by nullbuffer_add().
    for ( nbuf->flush_pos = 0; nbuf->flush_pos < nbuf->used; ++nbuf->flush_pos ) {
        struct __obj_with_rtti x = nbuf->objs[nbuf->flush_pos];

        if ( ! x.obj )
            // May have been removed.
            continue;

#ifdef DEBUG
        --__hlt_globals()->num_nullbuffer;
#endif

        __hlt_gchdr* hdr = (__hlt_gchdr*)x.obj;

        if ( hdr->ref_cnt > 0 )
            // Still alive actually.
            continue;

        if ( x.ti->obj_dtor )
            (*(x.ti->obj_dtor))(x.ti, x.obj, ctx);

        __hlt_free(x.obj, x.ti->tag, "nullbuffer_flush");
    }

    nbuf->used = 0;

    if ( nbuf->allocated > __INITIAL_NULLBUFFER_SIZE ) {
        hlt_free(nbuf->objs);
        nbuf->allocated = __INITIAL_NULLBUFFER_SIZE;
        nbuf->objs = (struct __obj_with_rtti*) hlt_malloc(sizeof(struct __obj_with_rtti) * nbuf->allocated);
    }

#ifdef DEBUG
    _dbg_mem_raw("nullbuffer_flush", nbuf, nbuf->used, 0, "end", 0, ctx);
#endif

   nbuf->flush_pos = -1;
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
    stats.num_nullbuffer = globals->num_nullbuffer;
    stats.max_nullbuffer = globals->max_nullbuffer;

    return stats;
}

