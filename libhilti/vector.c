//
// FIXME: Vector's don't shrink at the moment. When should they?
//
//

#include <stdio.h>

#include "autogen/hilti-hlt.h"
#include "type-info.h"
#include "vector.h"
#include "timer.h"
#include "interval.h"
#include "enum.h"
#include "int.h"

#include <string.h>

// Factor by which to growth array on reallocation.
static const float GrowthFactor = 1.5;

// Initial allocation size for a new vector
static const hlt_vector_idx InitialCapacity = 1;

struct __hlt_vector {
    __hlt_gchdr __gchdr;        // Header for memory management.
    void* elems;                // Pointer to the element array.
    hlt_timer** timers;         // Array of timers, with entries 0 if not set. Not memory-managed to avoid cycles.
    hlt_vector_idx last;        // Largest valid index.
    hlt_vector_idx capacity;    // Number of element we have physically allocated in elems.
    const hlt_type_info* type;  // Type information for our elements.
    void* def;                  // Default element for not yet initialized fields.
    hlt_timer_mgr* tmgr;        // The timer manager or null if none.
    hlt_interval timeout;       // The timeout value, or 0 if disabled.
    hlt_enum strategy;          // Expiration strategy if set; zero otherwise.
};

void hlt_vector_dtor(hlt_type_info* ti, hlt_vector* v)
{
    void* end = v->elems + v->last * v->type->size;
    for ( void* elem = v->elems; elem <= end; elem += v->type->size ) {
        GC_DTOR_GENERIC(elem, v->type);
    }

    GC_DTOR_GENERIC(v->def, v->type);
    GC_DTOR(v->tmgr, hlt_timer_mgr);

    hlt_free(v->elems);
    hlt_free(v->timers);
    hlt_free(v->def);
}

void hlt_iterator_vector_cctor(hlt_type_info* ti, hlt_iterator_vector* i)
{
    GC_CCTOR(i->vec, hlt_vector);
}

void hlt_iterator_vector_dtor(hlt_type_info* ti, hlt_iterator_vector* i)
{
    GC_DTOR(i->vec, hlt_vector);
}

static inline void _access_entry(hlt_vector* v, hlt_vector_idx i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! v->tmgr || ! hlt_enum_equal(v->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || v->timeout == 0 )
        return;

    if ( ! v->timers[i] )
        return;

    hlt_time t = hlt_timer_mgr_current(v->tmgr, excpt, ctx) + v->timeout;
    hlt_timer_update(v->timers[i], t, excpt, ctx);
}

// Val is not yet ref'ed.
static inline void _set_entry(hlt_vector* v, hlt_vector_idx i, void *val, int dtor, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // Cancel old timer if active.
    if ( v->tmgr && v->timers && v->timers[i] ) {
        hlt_timer_cancel(v->timers[i], excpt, ctx);
        v->timers[i] = 0;
    }

    void* dst = v->elems + i * v->type->size;

    if ( dtor )
        GC_DTOR_GENERIC(dst, v->type);

    memcpy(dst, val, v->type->size);
    GC_CCTOR_GENERIC(dst, v->type);

    // Start timer if needed.
    if ( v->tmgr && v->timeout ) {
        GC_CCTOR(v, hlt_vector);
        __hlt_vector_timer_cookie cookie = { v, i };
        v->timers[i] = __hlt_timer_new_vector(cookie, excpt, ctx); // Not memory-managed on our end.
        hlt_time t = hlt_timer_mgr_current(v->tmgr, excpt, ctx) + v->timeout;
        hlt_timer_mgr_schedule(v->tmgr, t, v->timers[i], excpt, ctx);
        GC_DTOR(v->timers[i], hlt_timer); // Not memory-managed on our end.
    }
}

hlt_vector* hlt_vector_new(const hlt_type_info* elemtype, const void* def, struct __hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_vector* v = GC_NEW(hlt_vector);
    v->elems = hlt_malloc(elemtype->size * InitialCapacity);

    if ( tmgr ) {
        GC_INIT(v->tmgr, tmgr, hlt_timer_mgr);
        v->timers = hlt_malloc(sizeof(hlt_timer*) *  InitialCapacity);
        assert(v->timers);
    }

    // We need to deep-copy the default element as the caller might have it
    // on its stack.
    v->def = hlt_malloc(elemtype->size);
    memcpy(v->def, def, elemtype->size);
    GC_CCTOR_GENERIC(v->def, elemtype);

    v->last = -1;
    v->capacity = InitialCapacity;
    v->type = elemtype;
    v->timeout = 0.0;
    v->strategy = hlt_enum_unset(excpt, ctx);

    return v;
}

void hlt_vector_timeout(hlt_vector* v, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! v->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    v->timeout = timeout;
    v->strategy = strategy;
}

void* hlt_vector_get(hlt_vector* v, hlt_vector_idx i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( i > v->last ) {
        hlt_set_exception(excpt, &hlt_exception_index_error, 0);
        return 0;
    }

    _access_entry(v, i, excpt, ctx);
    void *elem = v->elems + i * v->type->size;
    GC_CCTOR_GENERIC(elem, v->type);

    return elem;
}

void hlt_vector_set(hlt_vector* v, hlt_vector_idx i, const hlt_type_info* elemtype, void* val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(hlt_type_equal(v->type, elemtype));

    if ( i >= v->capacity ) {
        // Allocate more memory.
        hlt_vector_idx c = v->capacity;
        while ( i >= c )
            c *= (c+1) * GrowthFactor;

        hlt_vector_reserve(v, c, excpt, ctx);
    }

    for ( int j = v->last + 1; j <= i; j++ ) {
        // Initialize elements between old and new end of vector.
        void* dst = v->elems + j * v->type->size;
        memcpy(dst, v->def, v->type->size);
        GC_CCTOR_GENERIC(dst, v->type);

        if ( v->tmgr ) {
            assert(v->timers);
            v->timers[j] = 0;
        }
    }

    if ( i > v->last )
        v->last = i;

    _set_entry(v, i, val, 1, excpt, ctx);
}

void hlt_vector_push_back(hlt_vector* v, const hlt_type_info* elemtype, void* val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(v);
    assert(hlt_type_equal(v->type, elemtype));

    ++v->last;

    if ( v->last >= v->capacity ) {
        // Allocate more memory.
        hlt_vector_idx c = (v->capacity + 1) * GrowthFactor;
        hlt_vector_reserve(v, c, excpt, ctx);
    }

    assert(v->last < v->capacity);

    if ( v->tmgr ) {
        assert(v->timers);
        v->timers[v->last] = 0;
    }

    _set_entry(v, v->last, val, 0, excpt, ctx);
}

hlt_vector_idx hlt_vector_size(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return v->last + 1;
}

void hlt_vector_expire(__hlt_vector_timer_cookie cookie)
{
    hlt_vector* v = cookie.vec;
    hlt_vector_idx i = cookie.idx;

    if ( v->tmgr )
        v->timers[i] = 0;

    void* dst = v->elems + i * v->type->size;
    GC_DTOR_GENERIC(dst, v->type);
    memcpy(dst, v->def, v->type->size);
    GC_CCTOR_GENERIC(dst, v->type);
}

void hlt_vector_reserve(hlt_vector* v, hlt_vector_idx n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( v->capacity >= n )
        return;

    v->capacity = n;
    v->elems = hlt_realloc(v->elems, v->type->size * n);

    if ( v->timers ) {
        v->timers = hlt_realloc(v->timers, sizeof(hlt_timer*) * n);
        assert(v->timers);
    }
}

hlt_iterator_vector hlt_vector_begin(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_vector i;

    GC_INIT(i.vec, (v->last >= 0 ? v : 0), hlt_vector);
    i.idx = 0;

    return i;
}

hlt_iterator_vector hlt_vector_end(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_vector i;
    i.vec = 0;
    i.idx = 0;
    return i;
}

hlt_iterator_vector hlt_iterator_vector_incr(hlt_iterator_vector i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.vec )
        return i;

    hlt_iterator_vector j = i;
    ++j.idx;

    if ( j.idx > j.vec->last ) {
        // End reached.
        j.vec = 0;
        j.idx = 0;
    }

    GC_CCTOR(j, hlt_iterator_vector);
    return j;
}

void* hlt_iterator_vector_deref(hlt_iterator_vector i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.vec ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    _access_entry(i.vec, i.idx, excpt, ctx);
    void *elem = i.vec->elems + i.idx * i.vec->type->size;
    GC_CCTOR_GENERIC(elem, i.vec->type);
    return elem;
}

int8_t hlt_iterator_vector_eq(hlt_iterator_vector i1, hlt_iterator_vector i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( i1.vec )
        _access_entry(i1.vec, i1.idx, excpt, ctx);

    if ( i2.vec)
        _access_entry(i2.vec, i2.idx, excpt, ctx);

    return i1.vec == i2.vec && i1.idx == i2.idx;
}

hlt_string hlt_vector_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_vector* v = *((const hlt_vector**)obj);

    if ( ! v ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    hlt_string sep1 = hlt_string_from_asciiz(": ", excpt, ctx);
    hlt_string sep2 = hlt_string_from_asciiz(", ", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("[", excpt, ctx);

    for ( hlt_vector_idx i = 0; i <= v->last; i++ ) {

        hlt_string istr = hlt_int_to_string(&hlt_type_info_hlt_int_64, &i, options, excpt, ctx);

        s = hlt_string_concat_and_unref(s, istr, excpt, ctx);

        hlt_string tmp = s;
        s = hlt_string_concat(s, sep1, excpt, ctx);
        GC_DTOR(tmp, hlt_string);

        hlt_string t = 0;

        if ( v->type->to_string )
            t = (v->type->to_string)(v->type, v->elems + i * v->type->size, options, excpt, ctx);
        else
            // No format function.
            t = hlt_string_from_asciiz(v->type->tag, excpt, ctx);

        s = hlt_string_concat_and_unref(s, t, excpt, ctx);

        if ( i < v->last ) {
            hlt_string tmp = s;
            s = hlt_string_concat(s, sep2, excpt, ctx);
            GC_DTOR(tmp, hlt_string);
        }

        if ( *excpt )
            goto error;
    }

    hlt_string postfix = hlt_string_from_asciiz("]", excpt, ctx);
    s = hlt_string_concat_and_unref(s, postfix, excpt, ctx);

    GC_DTOR(sep1, hlt_string);
    GC_DTOR(sep2, hlt_string);
    return s;

error:
    GC_DTOR(sep1, hlt_string);
    GC_DTOR(sep2, hlt_string);
    GC_DTOR(s, hlt_string);
    return 0;
}

