//

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "autogen/hilti-hlt.h"
#include "bytes.h"
#include "exceptions.h"
#include "globals.h"
#include "hutil.h"
#include "int.h"
#include "memory_.h"
#include "string_.h"
#include "threading.h"

static const size_t __HLT_BYTES_MIN_RESERVE = 32;

// Bytes object cannot be changed anymore.
static const int _BYTES_FLAG_FROZEN = 1;

// Data of this node contains a separator object. It's fine to cast to
// __hlt_bytes_object in that case. The other data pointers inside the bytes
// object aren't valid in this case, and set to null.
static const int _BYTES_FLAG_OBJECT = 2;

// Layout here must match libhilti.ll!
struct __hlt_bytes {
    __hlt_gchdr __gchdr;                  // Header for memory management.
    __hlt_thread_mgr_blockable blockable; // For blocking until changed.
    int8_t flags;                         // Flags as combiniation of _BYTES_FLAG_* values.
    struct __hlt_bytes* next;             // Next part. Ref counted.
    hlt_bytes_size offset; // The offset of this chunks first byte relative to the beginning of the
                           // bytes object it's part of.
    int8_t* start;         // Pointer to first data byte.
    int8_t* end;           // Pointer to one after last data byte used so far.
    int8_t* reserved;      // Pointer to one after the last data byte available.
    int8_t* to_free;       // Need to free data pointed to when dtoring.
    hlt_bytes_size* marks; // If non-null, array of offsets of marks within this chunk. Terminated
                           // by -1. Must be freed.
    int8_t data[];         // Inline data starts here if free is zero.
};

// Specialized bytes object storing a separator object.
struct __hlt_bytes_object {
    struct __hlt_bytes b;      // Common header.
    const hlt_type_info* type; // Type of the object;
    char object[];             // Object's storage starts here, with size determined by type.
};

// Hoisted version when storing on the stack. Layout here must match
// libhilti.ll!
struct __hlt_bytes_hoisted {
    struct __hlt_bytes b;
    int8_t data[32];
};

typedef struct __hlt_bytes_object __hlt_bytes_object;

static hlt_iterator_bytes GenericEndPos = {0, 0};

static hlt_bytes* _hlt_bytes_new(const int8_t* data, hlt_bytes_size len, hlt_bytes_size reserve,
                                 hlt_execution_context* ctx);
static void __add_chunk(hlt_bytes* tail, hlt_bytes* c, hlt_execution_context* ctx);

static inline hlt_bytes_size min(hlt_bytes_size a, hlt_bytes_size b)
{
    return a < b ? a : b;
}

static inline __hlt_bytes_object* __get_object(const hlt_bytes* b)
{
    return (b && (b->flags & _BYTES_FLAG_OBJECT)) ? (__hlt_bytes_object*)b : 0;
}

static inline hlt_bytes* __tail(hlt_bytes* b, int8_t consider_object)
{
    if ( ! b )
        return 0;

    while ( b->next ) {
        if ( ! consider_object && __get_object(b->next) )
            break;

        b = b->next;
    }

    return b;
}

static inline int8_t __at_object(const hlt_iterator_bytes i)
{
    return i.bytes && __get_object(i.bytes) != 0;
}

static inline int8_t __at_mark(const hlt_iterator_bytes i)
{
    hlt_bytes* b = i.bytes;

    if ( ! (b && b->marks) )
        return 0;

    for ( hlt_bytes_size* m = b->marks; m && *m != -1; m++ ) {
        if ( *m == (i.cur - b->start) )
            return 1;
    }

    return 0;
}

static inline int8_t __is_end(const hlt_iterator_bytes p)
{
    return p.bytes == 0 || (p.bytes == __tail(p.bytes, false) && p.cur >= p.bytes->end) ||
           __at_object(p);
}

static inline int8_t __is_frozen(const hlt_bytes* b)
{
    return b ? (b->flags & _BYTES_FLAG_FROZEN) : false;
}

#if 0
// For debugging.
static void __print_bytes(const char* prefix, const hlt_bytes* b)
{
    if ( ! b ) {
        fprintf(stderr, "%s: (null)\n", prefix);
        return;
    }

    fprintf(stderr, "%s: %p b( ", prefix, b);
    for ( ; b; b = b->next ) {
        fprintf(stderr, "#%ld:%p-%p \"", b->end - b->start, b->start, b->end);

        if ( __get_object(b) ) {
            fprintf(stderr, " [[object]] ");
            continue;
        }

        for ( int i = 0; i < min(b->end - b->start, 20); i++ ) {
            if ( isprint(b->start[i]) )
                fprintf(stderr, "%c", b->start[i]);
            else
                fprintf(stderr, "\\x%02x", b->start[i]);
        }

        if ( (b->end - b->start) > 20 )
            fprintf(stderr, "...");

        fprintf(stderr, "\" ");

        if ( b->marks ) {
            fprintf(stderr, "[ ");

            for ( hlt_bytes_size* m = b->marks; m && *m >= 0; m++ )
                fprintf(stderr, "m@%" PRId64 " ", *m);

            fprintf(stderr, "] ");
        }
    }

    fprintf(stderr, ")\n");
}
#endif

#if 0
static void __print_iterator(const char* prefix, hlt_iterator_bytes i)
{
    fprintf(stderr, "%s: i(#%p@%lu) is_end=%d at_object=%d at_mark=%d\n", prefix, i.bytes,
            i.bytes ? i.cur - i.bytes->start : 0, __is_end(i), __at_object(i), __at_mark(i));

    __print_bytes("  -> ", i.bytes);
}
#endif

static inline int8_t __is_empty(const hlt_bytes* b, int8_t consider_objects)
{
    for ( const hlt_bytes* c = b; c; c = c->next ) {
        if ( __get_object(c) )
            return ! consider_objects;

        if ( c->start != c->end )
            return 0;
    }

    return 1;
}

// TODO: Frequent enough to want a double-linked list?
static inline hlt_bytes* __pred(hlt_bytes* b, hlt_bytes* p)
{
    for ( ; b && b->next != p; b = b->next )
        ;

    return b;
}

// Does not ref the iterator.
static inline hlt_iterator_bytes __create_iterator(hlt_bytes* bytes, int8_t* cur)
{
    hlt_iterator_bytes i = {bytes, cur};
    return i;
}

void __hlt_iterator_bytes_incr_by(hlt_iterator_bytes* p, int64_t n, hlt_exception** excpt,
                                  hlt_execution_context* ctx, int8_t adj_ref,
                                  int8_t move_beyond_end);
hlt_iterator_bytes hlt_bytes_offset(hlt_bytes* b, hlt_bytes_size p, hlt_exception** excpt,
                                    hlt_execution_context* ctx);

// This version does not adjust the reference count and must be called only
// when the potentiall changed iterator will not be visible to the HILTI
// layer.
static inline void __normalize_iter(hlt_iterator_bytes* pos)
{
    if ( ! pos->bytes || __at_object(*pos) )
        return;

    // If the pos was previously an end position but now new data has been
    // added, adjust it so that it's pointing to the next byte (or even
    // beyond that, if the previous iterator recorded a future position).
    // Note we do not adjust the reference count in this version.

    int64_t incr = 0;

    while ( pos->cur >= pos->bytes->end && pos->bytes->next && ! __at_object(*pos) ) {
        incr += (pos->cur - pos->bytes->end);
        pos->bytes = pos->bytes->next;
        pos->cur = pos->bytes->start;
    }

    if ( incr )
        __hlt_iterator_bytes_incr_by(pos, incr, 0, 0, 0, 1);
}

// This version adjust the reference count and should be called only when the
// potentiall changed iterator will be visible to the HILTI layer.
static inline void __normalize_iter_hilti(hlt_iterator_bytes* pos, hlt_execution_context* ctx)
{
    if ( ! pos->bytes || __at_object(*pos) )
        return;

    // If the pos was previously an end position but now new data has been
    // added, adjust it so that it's pointing to the next byte.

    int64_t incr = 0;

    while ( pos->cur >= pos->bytes->end && pos->bytes->next ) {
        incr += (pos->cur - pos->bytes->end);
        pos->bytes = pos->bytes->next;
        pos->cur = pos->bytes->start;
    }

    if ( incr )
        __hlt_iterator_bytes_incr_by(pos, incr, 0, 0, 1, 1);
}

hlt_bytes_size __hlt_bytes_len(hlt_bytes* b)
{
    hlt_bytes_size len = 0;

    for ( ; b && ! __get_object(b); b = b->next )
        len += (b->end - b->start);

    return len;
}

// This reservers enough space to fit all old plus addl_reserve new marks
// plus the final -1, and then returns a pointer to where the first new mark
// should be stored.
hlt_bytes_size* __hlt_bytes_reserve_space_for_more_marks(hlt_bytes_size** dst, int addl_reserve)
{
    assert(dst);

    if ( ! *dst ) {
        *dst = hlt_malloc((addl_reserve + 1) * sizeof(hlt_bytes_size));
        return *dst;
    }

    else {
        int len_dst = 0;
        for ( hlt_bytes_size *i = *dst; *i >= 0; i++, len_dst++ )
            ;
        *dst = hlt_realloc(*dst, (len_dst + addl_reserve + 1) * sizeof(hlt_bytes_size),
                           (len_dst + 1) * sizeof(hlt_bytes_size));
        return *dst + len_dst;
    }
}

void __hlt_bytes_append_mark(hlt_bytes* b, hlt_bytes_size mark, hlt_execution_context* ctx)
{
    hlt_bytes* tail = __tail(b, true);

    if ( __get_object(tail) ) {
        // Need to add an empty block to record the mark.
        hlt_bytes* empty = _hlt_bytes_new(0, 0, 0, ctx);
        __add_chunk(tail, empty, ctx);
        tail = empty;
    }

    hlt_bytes_size* dst = __hlt_bytes_reserve_space_for_more_marks(&tail->marks, 1);
    *dst++ = (mark >= 0 ? mark : (tail->end - tail->start));
    *dst++ = -1;
}

void __hlt_bytes_copy_marks(hlt_bytes_size** marks, hlt_bytes* b, int8_t* first, int8_t* last,
                            int8_t adjoffset)
{
    if ( ! (marks && b->marks) )
        return;

    int n = 0;
    for ( hlt_bytes_size *i = b->marks; *i != -1; i++, n++ )
        ;

    hlt_bytes_size* dst = __hlt_bytes_reserve_space_for_more_marks(marks, n);

    int64_t offset_first = first ? (first - b->start) : -1;
    int64_t offset_last = last ? (last - b->start) : -1;

    for ( hlt_bytes_size* p = b->marks; *p != -1; p++ ) {
        if ( *p < 0 )
            continue;

        hlt_bytes_size offset = *p + adjoffset;

        if ( offset_first >= 0 && offset < offset_first )
            continue;

        if ( offset_last >= 0 && offset >= offset_last )
            continue;

        *dst++ = offset;
    }

    *dst++ = -1;
}

// c not yet ref'ed.
static void __add_chunk(hlt_bytes* tail, hlt_bytes* c, hlt_execution_context* ctx)
{
    assert(c);
    assert(tail);
    assert(! tail->next);
    assert(! __is_frozen(tail));

    GC_CCTOR(c, hlt_bytes, ctx);
    tail->next = c;
    c->offset = tail->offset + (__get_object(tail) ? 0 : tail->end - tail->start);

    if ( tail->marks ) {
        for ( hlt_bytes_size* p = tail->marks; *p != -1; p++ ) {
            if ( *p >= (tail->end - tail->start) ) {
                __hlt_bytes_append_mark(tail, *p - (tail->end - tail->start), ctx);
                *p = -2; // Delete.
            }
        }
    }
}

static inline void _hlt_bytes_init(hlt_bytes* b, const int8_t* data, hlt_bytes_size len,
                                   hlt_bytes_size reserve, hlt_execution_context* ctx)
{
    assert(reserve >= len);

    b->flags = 0;
    b->next = 0;
    b->offset = 0;
    b->start = b->data;
    b->end = b->start + len;
    b->reserved = b->start + reserve;
    b->to_free = 0;
    b->marks = 0;

    hlt_thread_mgr_blockable_init(&b->blockable);

    if ( data )
        memcpy(b->data, data, len);
}

static inline void _hlt_bytes_init_reuse(hlt_bytes* b, int8_t* data, hlt_bytes_size len,
                                         hlt_execution_context* ctx)
{
    b->flags = 0;
    b->next = 0;
    b->offset = 0;
    b->start = data;
    b->end = data + len;
    b->reserved = data + len;
    b->to_free = data;
    b->marks = 0;

    hlt_thread_mgr_blockable_init(&b->blockable);
}

static void _hlt_bytes_init_object(__hlt_bytes_object* b, const hlt_type_info* type, void* obj,
                                   hlt_execution_context* ctx)
{
    b->b.next = 0;
    b->b.flags = _BYTES_FLAG_OBJECT;
    b->b.offset = 0;
    b->b.marks = 0;
    b->type = type;

    hlt_thread_mgr_blockable_init(&b->b.blockable);

    if ( obj ) {
        memcpy(&b->object, obj, type->size);
        GC_CCTOR_GENERIC(&b->object, type, ctx);
    }
}

static inline void _hlt_bytes_init_hoisted(__hlt_bytes_hoisted* dst, const int8_t* data,
                                           hlt_bytes_size len, hlt_execution_context* ctx)
{
    hlt_bytes* b = &dst->b;

    if ( b->to_free )
        // Previous use had allocated memory.
        //
        // TODO: Is it save to assume that a newly allocated instance will
        // have this cleared?
        hlt_free(b->to_free);

    if ( b->marks )
        // Previous use had allocated memory.
        hlt_free(b->marks);

    if ( len <= sizeof(dst->data) ) {
        b->start = dst->data;
        b->reserved = b->start + sizeof(dst->data);
        b->to_free = 0;
    }

    else {
        b->start = hlt_malloc(len);
        b->reserved = b->start + len;
        b->to_free = b->start;
    }

    b->flags = 0;
    b->offset = 0;
    b->next = 0;
    b->end = b->start + len;
    b->marks = 0;

    hlt_thread_mgr_blockable_init(&b->blockable);

    if ( data )
        memcpy(b->data, data, len);
}

static hlt_bytes* _hlt_bytes_new(const int8_t* data, hlt_bytes_size len, hlt_bytes_size reserve,
                                 hlt_execution_context* ctx)
{
    if ( ! reserve )
        reserve = (len ? len : __HLT_BYTES_MIN_RESERVE);

    hlt_bytes* b = GC_NEW_CUSTOM_SIZE_NO_INIT(hlt_bytes, sizeof(hlt_bytes) + reserve, ctx);
    _hlt_bytes_init(b, data, len, reserve, ctx);
    return b;
}

static hlt_bytes* _hlt_bytes_new_ref(const int8_t* data, hlt_bytes_size len, hlt_bytes_size reserve,
                                     hlt_execution_context* ctx)
{
    if ( ! reserve )
        reserve = (len ? len : __HLT_BYTES_MIN_RESERVE);

    hlt_bytes* b = GC_NEW_CUSTOM_SIZE_NO_INIT_REF(hlt_bytes, sizeof(hlt_bytes) + reserve, ctx);
    _hlt_bytes_init(b, data, len, reserve, ctx);
    return b;
}

static hlt_bytes* _hlt_bytes_new_reuse(int8_t* data, hlt_bytes_size len, hlt_execution_context* ctx)
{
    hlt_bytes* b = GC_NEW_NO_INIT(hlt_bytes, ctx);
    _hlt_bytes_init_reuse(b, data, len, ctx);
    return b;
}

#if 0
static hlt_bytes* _hlt_bytes_new_reuse_ref(int8_t* data, hlt_bytes_size len,
                                           hlt_execution_context* ctx)
{
    hlt_bytes* b = GC_NEW_NO_INIT_REF(hlt_bytes, ctx);
    _hlt_bytes_init_reuse(b, data, len, ctx);
    return b;
}
#endif

static hlt_bytes* _hlt_bytes_new_object(const hlt_type_info* type, void* obj,
                                        hlt_execution_context* ctx)
{
    __hlt_bytes_object* b =
        GC_NEW_CUSTOM_SIZE_NO_INIT(hlt_bytes, sizeof(__hlt_bytes_object) + type->size, ctx);
    _hlt_bytes_init_object(b, type, obj, ctx);
    return &b->b;
}

static hlt_bytes* _hlt_bytes_new_object_ref(const hlt_type_info* type, void* obj,
                                            hlt_execution_context* ctx)
{
    __hlt_bytes_object* b =
        GC_NEW_CUSTOM_SIZE_NO_INIT_REF(hlt_bytes, sizeof(__hlt_bytes_object) + type->size, ctx);
    _hlt_bytes_init_object(b, type, obj, ctx);
    return &b->b;
}

void hlt_bytes_dtor(hlt_type_info* ti, hlt_bytes* b, hlt_execution_context* ctx)
{
    b->start = b->end = 0;
    GC_CLEAR(b->next, hlt_bytes, ctx);

    __hlt_bytes_object* obj = __get_object(b);

    if ( obj ) {
        GC_DTOR_GENERIC(&obj->object, obj->type, ctx);
    }

    else {
        if ( b->to_free )
            hlt_free(b->to_free);

        if ( b->marks )
            hlt_free(b->marks);
    }
}

void hlt_iterator_bytes_dtor(hlt_type_info* ti, hlt_iterator_bytes* p, hlt_execution_context* ctx)
{
    GC_DTOR(p->bytes, hlt_bytes, ctx);
}

void hlt_iterator_bytes_cctor(hlt_type_info* ti, hlt_iterator_bytes* p, hlt_execution_context* ctx)
{
    GC_CCTOR(p->bytes, hlt_bytes, ctx);
}

void hlt_bytes_new_hoisted(__hlt_bytes_hoisted* dst, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    _hlt_bytes_init_hoisted(dst, 0, 0, ctx);
}

hlt_bytes* hlt_bytes_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return _hlt_bytes_new(0, 0, 0, ctx);
}

hlt_bytes* hlt_bytes_new_from_data(int8_t* data, hlt_bytes_size len, hlt_exception** excpt,
                                   hlt_execution_context* ctx)
{
    return _hlt_bytes_new_reuse(data, len, ctx);
}

void hlt_bytes_new_from_data_copy_hoisted(__hlt_bytes_hoisted* dst, const int8_t* data,
                                          hlt_bytes_size len, hlt_exception** excpt,
                                          hlt_execution_context* ctx)
{
    _hlt_bytes_init_hoisted(dst, data, len, ctx);
}

hlt_bytes* hlt_bytes_new_from_data_copy(const int8_t* data, hlt_bytes_size len,
                                        hlt_exception** excpt, hlt_execution_context* ctx)
{
    return _hlt_bytes_new(data, len, 0, ctx);
}

void* hlt_bytes_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate,
                            hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* src = *(hlt_bytes**)srcp;

    __hlt_bytes_object* so = __get_object(src);

    if ( so )
        return _hlt_bytes_new_object_ref(so->type, 0, ctx);
    else
        return _hlt_bytes_new_ref(0, src->end - src->start, 0, ctx);
}

void hlt_bytes_clone_init(void* dstp, const hlt_type_info* ti, void* srcp,
                          __hlt_clone_state* cstate, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    hlt_bytes* src = *(hlt_bytes**)srcp;
    hlt_bytes* dst = *(hlt_bytes**)dstp;

    assert(src && dst);

    dst->flags = src->flags;
    dst->offset = src->offset;
    dst->marks = 0;

    int first = 1;

    for ( ; src; src = src->next ) {
        hlt_bytes* b = 0;

        __hlt_bytes_object* so = __get_object(src);

        if ( so ) {
            if ( first ) {
                // Already allocated.
                __hlt_bytes_object* no = (__hlt_bytes_object*)dst;
                memcpy(&no->object, &so->object, no->type->size);
                GC_CCTOR_GENERIC(&no->object, no->type, ctx);
                b = &no->b;
            }

            else {
                void* p = &so->object;
                __hlt_bytes_object* no =
                    (__hlt_bytes_object*)_hlt_bytes_new_object(so->type, p, ctx);
                b = &no->b;
            }
        }

        else {
            if ( first ) {
                // Already allocated.
                hlt_bytes_size n = (src->end - src->start);
                memcpy(dst->start, src->start, n);
                b = dst;
            }

            else {
                hlt_bytes_size n = (src->end - src->start);
                b = _hlt_bytes_new(src->start, n, 0, ctx);
            }

            __hlt_bytes_copy_marks(&b->marks, src, 0, 0, 0);
        }

        if ( ! first ) {
            __add_chunk(dst, b, ctx);
            dst = b;
        }

        else
            first = 0;
    }
}

// Returns the number of bytes stored.
hlt_bytes_size hlt_bytes_len(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    return __hlt_bytes_len(b);
}

// Returns true if empty.
int8_t hlt_bytes_empty(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    return __is_empty(b, false);
}

void __hlt_bytes_append_raw(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt,
                            hlt_execution_context* ctx, int8_t reuse)
{
    if ( ! len ) {
        hlt_free(raw);
        return;
    }

    if ( __is_frozen(b) ) {
        hlt_free(raw);
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return;
    }

    // TODO: Copy into available space if large enough.

    hlt_bytes* c;

    if ( reuse )
        c = _hlt_bytes_new_reuse(raw, len, ctx);
    else
        c = _hlt_bytes_new(raw, len, 0, ctx);

    __add_chunk(__tail(b, true), c, ctx);

    hlt_thread_mgr_unblock(&b->blockable, ctx);
}

// Appends one Bytes object to another.
void __hlt_bytes_append(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt,
                        hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    if ( __is_frozen(b) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return;
    }

    hlt_bytes_size len = hlt_bytes_len(other, excpt, ctx);

    if ( ! len )
        return;

    // TODO: Copy into available space if large enough.

    hlt_bytes* dst = _hlt_bytes_new(0, len, 0, ctx);
    int8_t* p = dst->start;

    for ( ; other && ! __get_object(other); other = other->next ) {
        __hlt_bytes_copy_marks(&dst->marks, other, 0, 0, p - dst->start);
        hlt_bytes_size n = other->end - other->start;
        memcpy(p, other->start, n);
        p += n;
    }

    __add_chunk(__tail(b, true), dst, ctx);

    hlt_thread_mgr_unblock(&b->blockable, ctx);
}

void hlt_bytes_append(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt,
                      hlt_execution_context* ctx)
{
    __hlt_bytes_append(b, other, excpt, ctx);
}

void hlt_bytes_append_raw(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    __hlt_bytes_append_raw(b, raw, len, excpt, ctx, 1);
}

void hlt_bytes_append_raw_copy(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    __hlt_bytes_append_raw(b, raw, len, excpt, ctx, 0);
}

static void _hlt_bytes_concat_into(hlt_bytes* dst, hlt_bytes* b1, hlt_bytes* b2,
                                   hlt_exception** excpt, hlt_execution_context* ctx)
{
// Assumes that dst has enough space available.
#ifdef DEBUG
    hlt_bytes_size len1 = hlt_bytes_len(b1, excpt, ctx);
    hlt_bytes_size len2 = hlt_bytes_len(b2, excpt, ctx);
    assert(len1 + len2 <= (dst->reserved - dst->start));
    _UNUSED(len1);
    _UNUSED(len2);
#endif

    int8_t* p = dst->start;

    for ( ; b1 && ! __get_object(b1); b1 = b1->next ) {
        __hlt_bytes_copy_marks(&dst->marks, b1, 0, 0, p - dst->start);
        hlt_bytes_size n = b1->end - b1->start;
        memcpy(p, b1->start, n);
        p += n;
    }

    for ( ; b2 && ! __get_object(b2); b2 = b2->next ) {
        __hlt_bytes_copy_marks(&dst->marks, b2, 0, 0, p - dst->start);
        hlt_bytes_size n = b2->end - b2->start;
        memcpy(p, b2->start, n);
        p += n;
    }
}

void hlt_bytes_concat_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* b1, hlt_bytes* b2,
                              hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(dst);

    if ( ! (dst && b1 && b2) )
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);

    hlt_bytes_size len1 = hlt_bytes_len(b1, excpt, ctx);
    hlt_bytes_size len2 = hlt_bytes_len(b2, excpt, ctx);

    _hlt_bytes_init_hoisted(dst, 0, len1 + len2, ctx);
    _hlt_bytes_concat_into(&dst->b, b1, b2, excpt, ctx);
}

hlt_bytes* hlt_bytes_concat(hlt_bytes* b1, hlt_bytes* b2, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    if ( ! (b1 && b2) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len1 = hlt_bytes_len(b1, excpt, ctx);
    hlt_bytes_size len2 = hlt_bytes_len(b2, excpt, ctx);

    hlt_bytes* dst = _hlt_bytes_new(0, len1 + len2, 0, ctx);

    _hlt_bytes_concat_into(dst, b1, b2, excpt, ctx);

    return dst;
}

void __hlt_bytes_find_byte(hlt_iterator_bytes* p, hlt_bytes* b, int8_t chr, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    for ( ; b && ! __get_object(b); b = b->next ) {
        int8_t* c = memchr(b->start, chr, b->end - b->start);

        if ( c ) {
            *p = __create_iterator(b, c);
            return;
        }
    }

    *p = GenericEndPos;
}

hlt_iterator_bytes hlt_bytes_find_byte(hlt_bytes* b, int8_t chr, hlt_exception** excpt,
                                       hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return GenericEndPos;
    }

    hlt_iterator_bytes p;
    __hlt_bytes_find_byte(&p, b, chr, excpt, ctx);
    return p;
}

void __hlt_bytes_end(hlt_iterator_bytes* p, hlt_bytes* b, hlt_exception** excpt,
                     hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        *p = GenericEndPos;
        return;
    }

    p->bytes = __tail(b, false);
    p->cur = p->bytes->end;
}

void __hlt_bytes_begin(hlt_iterator_bytes* p, hlt_bytes* b, hlt_exception** excpt,
                       hlt_execution_context* ctx)
{
    if ( __is_empty(b, false) ) {
        __hlt_bytes_end(p, b, excpt, ctx);
        return;
    }

    p->bytes = b;
    p->cur = b->start;
    __normalize_iter(p);
}

void __hlt_iterator_bytes_incr(hlt_iterator_bytes* p, hlt_exception** excpt,
                               hlt_execution_context* ctx, int8_t adj_ref)
{
    if ( __is_end(*p) )
        // Fail silently.
        return;

    // Can we stay inside the same chunk?
    if ( p->cur + 1 < p->bytes->end ) {
        ++p->cur;
        return;
    }

    if ( ! p->bytes->next ) {
        // End reached.
        p->cur = p->bytes->end;
        return;
    }

    // Switch chunk.
    if ( adj_ref ) {
        p->bytes = p->bytes->next;
    }

    else
        p->bytes = p->bytes->next;

    p->cur = p->bytes->start;
}

void __hlt_iterator_bytes_incr_by(hlt_iterator_bytes* p, int64_t n, hlt_exception** excpt,
                                  hlt_execution_context* ctx, int8_t adj_ref,
                                  int8_t move_beyond_end)
{
    if ( __is_end(*p) )
        // Fail silently.
        return;

    if ( ! n )
        return;

    while ( 1 ) {
        // Can we stay inside the same chunk?
        if ( p->cur + n < p->bytes->end ) {
            p->cur += n;
            return;
        }

        if ( ! p->bytes->next || __get_object(p->bytes) ) {
            // End reached.
            if ( ! move_beyond_end )
                p->cur = p->bytes->end;
            else
                p->cur = p->bytes->end + n;

            return;
        }

        // Switch chunk.
        n -= (p->bytes->end - p->cur);

        if ( adj_ref ) {
            p->bytes = p->bytes->next;
        }

        else
            p->bytes = p->bytes->next;

        p->cur = p->bytes->start;
    }

    // Cannot reach.
    assert(false);
}

int8_t __hlt_iterator_bytes_deref(hlt_iterator_bytes p, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    if ( __is_end(p) ) {
        // Position is out range.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    return *p.cur;
}

int8_t __hlt_iterator_bytes_eq(hlt_iterator_bytes p1, hlt_iterator_bytes p2, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    if ( __is_end(p1) && __is_end(p2) )
        return 1;

    return p1.bytes == p2.bytes && p1.cur == p2.cur;
}

// Returns the number of bytes from p1 to p2 (not counting p2).
hlt_bytes_size __hlt_iterator_bytes_diff(hlt_iterator_bytes p1, hlt_iterator_bytes p2,
                                         hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( __is_end(p1) ) {
        if ( ! __is_end(p2) && ! (__at_object(p1) || __at_object(p2)) ) {
            // Invalid starting position.
            // hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx); // FIXME
        }

        return 0;
    }

    if ( __is_end(p2) )
        return hlt_bytes_len(p1.bytes, excpt, ctx) -
               (((p1.cur <= p1.bytes->end) ? p1.cur : p1.bytes->end) - p1.bytes->start);

    hlt_bytes_size n =
        (hlt_iterator_bytes_index(p2, excpt, ctx) - hlt_iterator_bytes_index(p1, excpt, ctx));
    return (n > 0 ? n : 0);
}

void __hlt_bytes_find_byte_from(hlt_iterator_bytes* p, hlt_iterator_bytes i, int8_t chr,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    // First chunk.
    hlt_bytes* b = i.bytes;
    int8_t* c = memchr(i.cur, chr, b->end - i.cur);

    if ( ! c ) {
        // Subsequent chunks.
        for ( b = i.bytes->next; b && ! __get_object(b); b = b->next ) {
            int8_t* c = memchr(b->start, chr, b->end - b->start);

            if ( c )
                break;
        }
    }

    if ( c )
        *p = __create_iterator(b, c);
    else
        __hlt_bytes_end(p, i.bytes, excpt, ctx);
}

int8_t __hlt_bytes_find_bytes(hlt_iterator_bytes* p, hlt_bytes* b, hlt_bytes* other,
                              hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( __is_empty(other, false) ) {
        // Empty pattern returns start position.
        __hlt_bytes_begin(p, b, excpt, ctx);
        return 1;
    }

    hlt_iterator_bytes begin;
    hlt_iterator_bytes end;
    __hlt_bytes_begin(&begin, other, excpt, ctx);
    __hlt_bytes_end(&end, other, excpt, ctx);


    int8_t chr = *begin.cur;

    hlt_iterator_bytes i;
    __hlt_bytes_begin(&i, b, excpt, ctx);

    while ( 1 ) {
        __hlt_bytes_find_byte_from(p, i, chr, excpt, ctx);

        if ( __is_end(*p) ) {
            // Not found.
            break;
        }

        if ( hlt_bytes_match_at(*p, other, excpt, ctx) )
            // Found.
            return 1;

        __hlt_iterator_bytes_incr(&i, excpt, ctx, 0);

        if ( __is_end(i) )
            // Not found.
            break;
    }

    __hlt_bytes_end(p, b, excpt, ctx);
    return 0;
}

int8_t hlt_bytes_contains_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    if ( ! (b && other) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_iterator_bytes p;
    return __hlt_bytes_find_bytes(&p, b, other, excpt, ctx);
}

hlt_iterator_bytes hlt_bytes_find_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt,
                                        hlt_execution_context* ctx)
{
    if ( ! (b && other) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return GenericEndPos;
    }

    hlt_iterator_bytes p;
    __hlt_bytes_find_bytes(&p, b, other, excpt, ctx);
    return p;
}

static int8_t __hlt_bytes_match_at(hlt_iterator_bytes p, hlt_bytes* b, hlt_exception** excpt,
                                   hlt_execution_context* ctx)
{
    if ( __is_end(p) )
        return __is_empty(b, false);

    if ( __is_empty(b, false) )
        return 0;

    hlt_iterator_bytes c1 = p;
    hlt_iterator_bytes c2;

    __hlt_bytes_begin(&c2, b, excpt, ctx);

    while ( 1 ) {
        // Extract bytes.
        int8_t b1 = *(c1.cur);
        int8_t b2 = *(c2.cur);

        if ( b1 != b2 )
            return 0;

        __hlt_iterator_bytes_incr(&c1, excpt, ctx, 0);
        __hlt_iterator_bytes_incr(&c2, excpt, ctx, 0);

        // End of pattern reached?
        if ( __is_end(c2) )
            // Found.
            return 1;

        // End of input reached?
        if ( __is_end(c1) )
            // Not found, but could have if more input available.
            return -1;
    }

    // Can't be reached.
    assert(0);
}

hlt_bytes_find_at_iter_result hlt_bytes_find_bytes_at_iter(hlt_iterator_bytes i, hlt_bytes* needle,
                                                           hlt_exception** excpt,
                                                           hlt_execution_context* ctx)
{
    hlt_bytes_find_at_iter_result r;
    r.iter = i;

    if ( ! needle ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        r.iter = GenericEndPos;
        return r;
    }

    if ( __is_empty(needle, false) ) {
        // Empty pattern returns start position.
        r.success = 1;
        r.iter = i;
        return r;
    }

    hlt_iterator_bytes nbegin;
    __hlt_bytes_begin(&nbegin, needle, excpt, ctx);
    int8_t chr = *nbegin.cur;

    __normalize_iter(&r.iter);

    while ( 1 ) {
        __hlt_bytes_find_byte_from(&r.iter, r.iter, chr, excpt, ctx);

        if ( __is_end(r.iter) ) {
            // Not found. Leave r.iter at end.
            r.success = 0;
            break;
        }

        int rc = __hlt_bytes_match_at(r.iter, needle, excpt, ctx);

        if ( rc > 0 ) {
            // Found. Leave r.iter at start position.
            r.success = 1;
            break;
        }

        if ( rc == -1 ) {
            // Out of input, may still match. Leave r.iter at start position.
            r.success = 0;
            break;
        }

        // Not found, advance.

        __hlt_iterator_bytes_incr(&r.iter, excpt, ctx, 0);

        if ( __is_end(r.iter) ) {
            // Not found. Leave r.iter at end.
            r.success = 0;
            break;
        }
    }

    return r;
}

static void _hlt_bytes_copy_into(hlt_bytes* dst, hlt_bytes* b, hlt_exception** excpt,
                                 hlt_execution_context* ctx)
{
    int8_t* p = dst->start;

    for ( ; b; b = b->next ) {
        __hlt_bytes_copy_marks(&dst->marks, b, 0, 0, p - dst->start);
        hlt_bytes_size n = (b->end - b->start);
        memcpy(p, b->start, n);
        p += n;
    }
}

void hlt_bytes_copy_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* b, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    assert(dst);

    if ( ! b )
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    _hlt_bytes_init_hoisted(dst, 0, len, ctx);

    _hlt_bytes_copy_into(&dst->b, b, excpt, ctx);
}

hlt_bytes* hlt_bytes_copy(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes* dst = _hlt_bytes_new(0, len, 0, ctx);

    _hlt_bytes_copy_into(dst, b, excpt, ctx);

    return dst;
}

hlt_bytes* hlt_bytes_clone(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* dst = 0;
    hlt_clone_deep(&dst, &hlt_type_info_hlt_bytes, &b, excpt, ctx);
    return dst;
}

static int8_t* __hlt_bytes_sub_raw_internal(int8_t* buffer, hlt_bytes_size** marks,
                                            hlt_bytes_size buffer_size, hlt_bytes_size len,
                                            hlt_iterator_bytes p1, hlt_iterator_bytes p2,
                                            hlt_exception** excpt, hlt_execution_context* ctx)
{
    // Some here is a bit awkward and only to retain the old semantics of
    // to_raw_buffer() that if the buffer gets exceeded, we fill it to the
    // max and return null; and a pointer to one beyond last byte written
    // otherwise.

    if ( ! buffer_size )
        return len ? 0 : buffer;

    if ( ! len )
        return buffer;

    if ( __is_end(p1) && ! __at_object(p1) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    if ( ! p2.bytes )
        __hlt_bytes_end(&p2, p1.bytes, excpt, ctx);

    int8_t* p = buffer;

    // Special case: both inside same chunk.
    if ( p1.bytes == p2.bytes ) {
        hlt_bytes_size n = min((p2.cur - p1.cur), buffer_size);

        if ( n )
            memcpy(p, p1.cur, n);

        __hlt_bytes_copy_marks(marks, p1.bytes, p1.cur, p2.cur, (p1.bytes->start - p1.cur));

        return buffer_size <= len ? p + n : 0;
    }

    // First block.
    hlt_bytes_size n = min((p1.bytes->end - p1.cur), buffer_size - (p - buffer));

    if ( n ) {
        __hlt_bytes_copy_marks(marks, p1.bytes, p1.cur, 0,
                               (p - buffer) + (p1.bytes->start - p1.cur));
        memcpy(p, p1.cur, n);
        p += n;
    }

    // Intermediate blocks.
    int8_t p2_is_end = __is_end(p2);
    hlt_bytes* c = p1.bytes->next;

    for ( ; c && (c != p2.bytes || p2_is_end) && ! __get_object(c); c = c->next ) {
        hlt_bytes_size n = min((c->end - c->start), buffer_size - (p - buffer));

        __hlt_bytes_copy_marks(marks, c, 0, 0, (p - buffer));

        if ( n ) {
            memcpy(p, c->start, n);
            p += n;
        }
    }

    if ( c && ! __get_object(c) ) {
        hlt_bytes_size n = min((p2.cur - p2.bytes->start), buffer_size - (p - buffer));

        __hlt_bytes_copy_marks(marks, p2.bytes, 0, p1.cur,
                               (p - buffer) + (p2.bytes->start - p2.cur));

        if ( n ) {
            memcpy(p, p2.bytes->start, n);
            p += n;
        }
    }

    else if ( ! p2_is_end && ! __get_object(c) ) {
        // Incompatible iterators.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    return buffer_size <= len ? p : 0;
}

void hlt_bytes_sub_hoisted(__hlt_bytes_hoisted* dst, hlt_iterator_bytes p1, hlt_iterator_bytes p2,
                           hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&p1);
    __normalize_iter(&p2);

    hlt_bytes_size len = __hlt_iterator_bytes_diff(p1, p2, excpt, ctx);
    _hlt_bytes_init_hoisted(dst, 0, len, ctx);

    if ( ! len )
        return;

    __hlt_bytes_sub_raw_internal(dst->b.start, &dst->b.marks, len, len, p1, p2, excpt, ctx);
}

hlt_bytes* hlt_bytes_sub(hlt_iterator_bytes p1, hlt_iterator_bytes p2, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    __normalize_iter(&p1);
    __normalize_iter(&p2);

    hlt_bytes_size len = __hlt_iterator_bytes_diff(p1, p2, excpt, ctx);
    hlt_bytes* dst = _hlt_bytes_new(0, len, 0, ctx);

    if ( ! len )
        return dst;

    if ( __hlt_bytes_sub_raw_internal(dst->start, &dst->marks, len, len, p1, p2, excpt, ctx) )
        return dst;

    return 0;
}

int8_t* hlt_bytes_sub_raw(int8_t* dst, size_t dst_len, hlt_iterator_bytes p1, hlt_iterator_bytes p2,
                          hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&p1);
    __normalize_iter(&p2);

    hlt_bytes_size len = __hlt_iterator_bytes_diff(p1, p2, excpt, ctx);

    if ( __hlt_bytes_sub_raw_internal(dst, 0, dst_len, len, p1, p2, excpt, ctx) )
        return dst;

    // Not enough space.
    return 0;
}

int8_t hlt_bytes_match_at(hlt_iterator_bytes p, hlt_bytes* b, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    __normalize_iter(&p);

    return __hlt_bytes_match_at(p, b, excpt, ctx) > 0;
}

int8_t* hlt_bytes_to_raw(int8_t* dst, size_t dst_len, hlt_bytes* b, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_iterator_bytes begin;
    hlt_iterator_bytes end;

    __hlt_bytes_begin(&begin, b, excpt, ctx);
    __hlt_bytes_end(&end, b, excpt, ctx);

    return hlt_bytes_sub_raw(dst, dst_len, begin, end, excpt, ctx);
}

int8_t __hlt_bytes_extract_one_slowpath(hlt_iterator_bytes* p, hlt_iterator_bytes end,
                                        hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(p);
    assert(p->bytes);

    __normalize_iter_hilti(p, ctx);
    __normalize_iter(&end);

    if ( __is_end(*p) || hlt_iterator_bytes_eq(*p, end, excpt, ctx) ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0, ctx);
        return 0;
    }

    int8_t b = __hlt_iterator_bytes_deref(*p, excpt, ctx);
    __hlt_iterator_bytes_incr(p, excpt, ctx, 1);

    return b;
}

// We optimize this for the common case and keep it small for inlining.
int8_t __hlt_bytes_extract_one(hlt_iterator_bytes* p, hlt_iterator_bytes end, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    if ( p->bytes && ! __get_object(p->bytes) ) {
        if ( (p->bytes == end.bytes && (p->cur < end.cur - 1)) ||
             (p->bytes != end.bytes && (p->cur < p->bytes->end - 1)) )
            return *(p->cur++);
    }

    return __hlt_bytes_extract_one_slowpath(p, end, excpt, ctx);
}

hlt_iterator_bytes hlt_bytes_offset(hlt_bytes* b, hlt_bytes_size p, hlt_exception** excpt,
                                    hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return GenericEndPos;
    }

    if ( ! p )
        return hlt_bytes_begin(b, excpt, ctx);

    if ( p < 0 )
        p += hlt_bytes_len(b, excpt, ctx);

    if ( p < 0 )
        return hlt_bytes_end(b, excpt, ctx);

    hlt_bytes* c;
    for ( c = b; c && p >= (c->end - c->start) && ! __get_object(c); c = c->next ) {
        p -= (c->end - c->start);
    }

    if ( ! c ) {
        // Position is out of range, return and end iterator that still
        // records the number of missing bytes in the cur field.
        hlt_iterator_bytes i;
        i.bytes = __tail(b, false);
        i.cur = i.bytes->end + p;
        return i;
    }

    hlt_iterator_bytes i;
    i.bytes = c;
    i.cur = c->start + p;
    return i;
}

void __hlt_bytes_normalize_iter(hlt_iterator_bytes* pos)
{
    __normalize_iter(pos);
}

hlt_iterator_bytes hlt_bytes_begin(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return GenericEndPos;
    }

    hlt_iterator_bytes p;
    __hlt_bytes_begin(&p, b, excpt, ctx);
    return p;
}

hlt_iterator_bytes hlt_bytes_end(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return GenericEndPos;
    }

    hlt_iterator_bytes p;
    __hlt_bytes_end(&p, b, excpt, ctx);
    return p;
}

hlt_iterator_bytes hlt_bytes_generic_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GenericEndPos;
}

void hlt_bytes_freeze(hlt_bytes* b, int8_t freeze, hlt_exception** excpt,
                      hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    for ( hlt_bytes* c = b; c; c = c->next ) {
        if ( freeze )
            c->flags |= _BYTES_FLAG_FROZEN;
        else
            c->flags &= ~_BYTES_FLAG_FROZEN;
    }

    hlt_thread_mgr_unblock(&b->blockable, ctx);
}

void hlt_bytes_trim(hlt_bytes* b, hlt_iterator_bytes p, hlt_exception** excpt,
                    hlt_execution_context* ctx)
{
    __normalize_iter(&p);

    __hlt_bytes_object* o = p.bytes ? __get_object(p.bytes) : 0;

    if ( __is_end(p) && ! o )
        return;

    // Check if within first block, we just adjust the start pointer there
    // then.
    if ( p.bytes == b ) {
        b->offset += (p.cur - b->start);
        b->start = p.cur;
        return;
    }

    // Find predecesor.
    hlt_bytes* pred = __pred(b, p.bytes);

    if ( ! pred ) {
        // Invalid iterator for this object.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return;
    }

    // We need to keep the start block so that our object pointer remains the
    // same, but we empty it out and then delete intermediary blocks.
    b->offset += (b->end - b->start);
    b->start = b->end;
    GC_ASSIGN(b->next, p.bytes, hlt_bytes, ctx);
    b->next->offset += (p.cur - b->next->start);
    b->next->start = p.cur;

    // Don't need old start node data anymore;
    if ( (o = __get_object(b)) ) {
        GC_DTOR_GENERIC(&o->object, o->type, ctx);
    }

    else if ( b->to_free ) {
        hlt_free(b->to_free);
        b->to_free = 0;

        if ( b->marks )
            hlt_free(b->marks);
    }
}

int8_t hlt_bytes_is_frozen(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    return __is_frozen(b);
}

void hlt_iterator_bytes_clone_init(void* dstp, const hlt_type_info* ti, void* srcp,
                                   __hlt_clone_state* cstate, hlt_exception** excpt,
                                   hlt_execution_context* ctx)
{
    hlt_iterator_bytes* src = (hlt_iterator_bytes*)srcp;
    hlt_iterator_bytes* dst = (hlt_iterator_bytes*)srcp;

    // We can only clone the and eob iterator.
    if ( __is_end(*src) ) {
        *dst = GenericEndPos;
        return;
    }

    hlt_string msg =
        hlt_string_from_asciiz("iterator<bytes> supports cloning only for end position", excpt,
                               ctx);
    hlt_set_exception(excpt, &hlt_exception_cloning_not_supported, msg, ctx);
}

hlt_iterator_bytes hlt_iterator_bytes_copy(hlt_iterator_bytes pos, hlt_exception** excpt,
                                           hlt_execution_context* ctx)
{
    __normalize_iter(&pos);

    if ( __is_end(pos) )
        return GenericEndPos;

    hlt_iterator_bytes copy;
    copy.bytes = pos.bytes;
    copy.cur = pos.cur;
    return copy;
}

int8_t hlt_iterator_bytes_is_frozen(hlt_iterator_bytes pos, hlt_exception** excpt,
                                    hlt_execution_context* ctx)
{
    __normalize_iter(&pos);

    if ( ! pos.bytes )
        return 0;

    return __is_frozen(pos.bytes);
}

int8_t hlt_iterator_bytes_deref(hlt_iterator_bytes pos, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    __normalize_iter(&pos);
    return __hlt_iterator_bytes_deref(pos, excpt, ctx);
}

hlt_iterator_bytes hlt_iterator_bytes_incr(hlt_iterator_bytes old, hlt_exception** excpt,
                                           hlt_execution_context* ctx)
{
    __normalize_iter(&old);

    hlt_iterator_bytes ni = old;
    __hlt_iterator_bytes_incr(&ni, excpt, ctx, 1);
    return ni;
}

hlt_iterator_bytes hlt_iterator_bytes_incr_by(hlt_iterator_bytes old, int64_t n,
                                              hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&old);

    hlt_iterator_bytes ni = old;
    __hlt_iterator_bytes_incr_by(&ni, n, excpt, ctx, 1, 0);

    return ni;
}

#if 0
// This is a bit tricky because pos_end() doesn't return anything suitable
// for iteration.  Let's leave this out for now until we know whether we
// actually need reverse iteration.
void hlt_iterator_bytes_decr(hlt_iterator_bytes* pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    XXX
}
#endif

int8_t hlt_iterator_bytes_eq(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2,
                             hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&pos1);
    __normalize_iter(&pos2);

    return __hlt_iterator_bytes_eq(pos1, pos2, excpt, ctx);
}

// Returns the number of bytes from pos1 to pos2 (not counting pos2).
hlt_bytes_size hlt_iterator_bytes_diff(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2,
                                       hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&pos1);
    __normalize_iter(&pos2);

    return __hlt_iterator_bytes_diff(pos1, pos2, excpt, ctx);
}

hlt_string hlt_bytes_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                               __hlt_pointer_stack* seen, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    hlt_string dst = 0;

    hlt_bytes* b = *((hlt_bytes**)obj);

    if ( ! b )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    char hex[5] = {'\\', 'x', 'X', 'X', '\0'};
    char buffer[256];
    uint64_t i = 0;

    for ( const hlt_bytes* c = b; c; c = c->next ) {
        __hlt_bytes_object* o = __get_object(c);

        if ( o ) {
            hlt_string prefix = hlt_string_from_asciiz("<obj:", excpt, ctx);
            hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);
            hlt_string tmp = hlt_object_to_string(o->type, &o->object, 0, excpt, ctx);

            dst = hlt_string_concat(dst, prefix, excpt, ctx);
            dst = hlt_string_concat(dst, tmp, excpt, ctx);
            dst = hlt_string_concat(dst, postfix, excpt, ctx);
            continue;
        }

        for ( int8_t* p = c->start; p < c->end; ++p ) {
            if ( i > sizeof(buffer) - 10 ) {
                hlt_string tmp = hlt_string_from_data((int8_t*)buffer, i, excpt, ctx);
                dst = hlt_string_concat(dst, tmp, excpt, ctx);
                i = 0;
            }

            unsigned char c = *(unsigned char*)p;

            if ( isprint((char)c) && c < 128 )
                buffer[i++] = c;

            else {
                hlt_util_uitoa_n(c, hex + 2, 3, 16, 1);
                strcpy(buffer + i, hex);
                i += sizeof(hex) - 1;
            }
        }

        if ( i ) {
            hlt_string tmp = hlt_string_from_data((int8_t*)buffer, i, excpt, ctx);
            dst = hlt_string_concat(dst, tmp, excpt, ctx);
            i = 0;
        }
    }

    return dst;
}

hlt_hash hlt_bytes_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt,
                        hlt_execution_context* ctx)
{
    hlt_bytes* b = *((hlt_bytes**)obj);

    hlt_hash hash = 0;

    for ( ; b; b = b->next ) {
        __hlt_bytes_object* o = __get_object(b);

        if ( o ) {
            hash += (o->type->hash)(o->type, &o->object, 0, 0);
            continue;
        }

        hlt_bytes_size n = (b->end - b->start);

        if ( n )
            hash = hlt_hash_bytes(b->start, n, hash);
    }

    return hash;
}

int8_t hlt_bytes_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2,
                       const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b1 = *((hlt_bytes**)obj1);
    hlt_bytes* b2 = *((hlt_bytes**)obj2);

    hlt_execution_context* c = hlt_global_execution_context();
    hlt_exception* e = 0;

    return hlt_bytes_cmp(b1, b2, &e, c) == 0;
}

void* hlt_bytes_blockable(const hlt_type_info* type, const void* obj, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    hlt_bytes* b = *((hlt_bytes**)obj);
    return &b->blockable;
}

void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_iterator_bytes p1,
                            hlt_iterator_bytes p2, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    __normalize_iter(&p1);
    __normalize_iter(&p2);

    if ( ! cookie ) {
        if ( __hlt_iterator_bytes_eq(p1, p2, excpt, ctx) ) {
            block->start = block->end = p1.cur;
            block->next = 0;
            return 0;
        }

        if ( __is_end(p1) && ! __is_end(p2) ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
            return 0;
        }

        if ( p1.bytes != p2.bytes ) {
            block->start = p1.cur;
            block->end = p1.bytes->end;
            block->next = p1.bytes->next;
        }

        else {
            block->start = p1.cur;
            block->end = p2.cur;
            block->next = 0;
        }
    }

    else {
        if ( ! block->next || __get_object(block->next) )
            return 0;

        if ( block->next != p2.bytes ) {
            block->start = block->next->start;
            block->end = block->next->end;
            block->next = block->next->next;
        }

        else {
            block->start = p2.bytes->start;
            block->end = p2.cur;
            block->next = 0;
        }
    }

    // We don't care what we return if we haven't reached the end yet as long
    // as it's not NULL.
    return block->next && ! __get_object(block->next) ? block : 0;
}

int8_t hlt_bytes_cmp(hlt_bytes* b1, hlt_bytes* b2, hlt_exception** excpt,
                     hlt_execution_context* ctx)
{
    if ( ! (b1 && b2) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    // Bail out quickly if one is empty but the other isn't.
    if ( __is_empty(b1, false) )
        return __is_empty(b2, false) ? 0 : 1;

    if ( __is_empty(b2, false) )
        return __is_empty(b1, false) ? 0 : -1;

    // FIXME: This is not the smartest implementation. Rather than using the
    // iterator functions, we should compare larger chunks at once.
    hlt_iterator_bytes p1;
    hlt_iterator_bytes p2;

    __hlt_bytes_begin(&p1, b1, excpt, ctx);
    __hlt_bytes_begin(&p2, b2, excpt, ctx);

    while ( 1 ) {
        if ( __is_end(p1) )
            return (__is_end(p2) ? 0 : 1);

        if ( __is_end(p2) )
            return -1;

        int8_t c1 = __hlt_iterator_bytes_deref(p1, excpt, ctx);
        int8_t c2 = __hlt_iterator_bytes_deref(p2, excpt, ctx);

        if ( c1 != c2 )
            return (c1 > c2 ? -1 : 1);

        __hlt_iterator_bytes_incr(&p1, excpt, ctx, 0);
        __hlt_iterator_bytes_incr(&p2, excpt, ctx, 0);
    }

    // Cannot be reached.
    assert(0);
}

int64_t hlt_bytes_to_int(hlt_bytes* b, int64_t base, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_iterator_bytes cur;
    __hlt_bytes_begin(&cur, b, excpt, ctx);

    int64_t value = 0;
    int64_t t = 0;
    int8_t first = 1;
    int8_t negate = 0;

    while ( ! __is_end(cur) ) {
        int8_t ch = __hlt_iterator_bytes_deref(cur, excpt, ctx);

        if ( isdigit(ch) )
            t = ch - '0';

        else if ( isalpha(ch) )
            t = 10 + tolower(ch) - 'a';

        else if ( first && ch == '-' )
            negate = 1;

        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
            return 0;
        }

        if ( t >= base ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
            return 0;
        }

        value = (value * base) + t;
        first = 0;

        __hlt_iterator_bytes_incr(&cur, excpt, ctx, 0);
    }

    return negate ? -value : value;
}

int64_t hlt_bytes_to_uint_binary(hlt_bytes* b, hlt_enum byte_order, hlt_exception** excpt,
                                 hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);

    if ( len > 8 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    hlt_iterator_bytes cur;
    __hlt_bytes_begin(&cur, b, excpt, ctx);

    int64_t i = 0;

    while ( ! __is_end(cur) ) {
        uint8_t ch = __hlt_iterator_bytes_deref(cur, excpt, ctx);
        i = (i << 8) | ch;
        __hlt_iterator_bytes_incr(&cur, excpt, ctx, 0);
    }

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Little, excpt, ctx) )
        i = hlt_int_flip(i, len, excpt, ctx);

    return i;
}

int64_t hlt_bytes_to_int_binary(hlt_bytes* b, hlt_enum byte_order, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);

    if ( len > 8 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    hlt_iterator_bytes cur;
    __hlt_bytes_begin(&cur, b, excpt, ctx);

    int64_t i = 0;

    while ( ! __is_end(cur) ) {
        uint8_t ch = __hlt_iterator_bytes_deref(cur, excpt, ctx);
        i = (i << 8) | ch;
        __hlt_iterator_bytes_incr(&cur, excpt, ctx, 0);
    }

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Little, excpt, ctx) )
        i = hlt_int_flip(i, len, excpt, ctx);

    if ( i & (1ll << (len * 8 - 1)) ) {
        if ( len == 8 )
            return -(~i + 1);

        return -(i ^ (1ll << (len * 8)) - 1) - 1;
    }

    return i;
}

static void _hlt_bytes_upper_into(hlt_bytes* dst, hlt_bytes* b, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    int8_t* p = dst->start;

    hlt_iterator_bytes cur;
    __hlt_bytes_begin(&cur, b, excpt, ctx);

    for ( hlt_bytes* c = b; c && ! __get_object(c); c = c->next ) {
        __hlt_bytes_copy_marks(&dst->marks, c, 0, 0, p - dst->start);

        for ( int8_t* s = c->start; s < c->end; s++ )
            *p++ = toupper(*s);
    }
}

void hlt_bytes_upper_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* b, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    if ( ! b )
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    _hlt_bytes_init_hoisted(dst, 0, len, ctx);
    _hlt_bytes_upper_into(&dst->b, b, excpt, ctx);
}

hlt_bytes* hlt_bytes_upper(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes* dst = _hlt_bytes_new(0, len, 0, ctx);

    _hlt_bytes_upper_into(dst, b, excpt, ctx);

    return dst;
}

static void _hlt_bytes_lower_into(hlt_bytes* dst, hlt_bytes* b, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    int8_t* p = dst->start;

    hlt_iterator_bytes cur;
    __hlt_bytes_begin(&cur, b, excpt, ctx);

    for ( hlt_bytes* c = b; c && ! __get_object(c); c = c->next ) {
        __hlt_bytes_copy_marks(&dst->marks, c, 0, 0, p - dst->start);

        for ( int8_t* s = c->start; s < c->end; s++ )
            *p++ = tolower(*s);
    }
}

void hlt_bytes_lower_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* b, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    if ( ! b )
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    _hlt_bytes_init_hoisted(dst, 0, len, ctx);
    _hlt_bytes_lower_into(&dst->b, b, excpt, ctx);
}

hlt_bytes* hlt_bytes_lower(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes* dst = _hlt_bytes_new(0, len, 0, ctx);

    _hlt_bytes_lower_into(dst, b, excpt, ctx);

    return dst;
}

int8_t hlt_bytes_starts_with(hlt_bytes* b, hlt_bytes* s, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    if ( ! (b && s) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    // FIXME: This is not the smartest implementation. Rather than using the
    // iterator functions, we should compare larger chunks at once.

    hlt_bytes_size len_b = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes_size len_s = hlt_bytes_len(s, excpt, ctx);

    if ( len_s > len_b )
        return 0;

    hlt_iterator_bytes cur_b;
    hlt_iterator_bytes cur_s;
    __hlt_bytes_begin(&cur_b, b, excpt, ctx);
    __hlt_bytes_begin(&cur_s, s, excpt, ctx);

    while ( ! __is_end(cur_s) ) {
        int8_t c1 = __hlt_iterator_bytes_deref(cur_b, excpt, ctx);
        int8_t c2 = __hlt_iterator_bytes_deref(cur_s, excpt, ctx);

        if ( c1 != c2 )
            return 0;

        __hlt_iterator_bytes_incr(&cur_b, excpt, ctx, 0);
        __hlt_iterator_bytes_incr(&cur_s, excpt, ctx, 0);
    }

    return 1;
}

static int8_t split1(hlt_bytes_pair* result, hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt,
                     hlt_execution_context* ctx)
{
    hlt_bytes_size sep_len = hlt_bytes_len(sep, excpt, ctx);

    hlt_iterator_bytes i;
    hlt_iterator_bytes j;

    if ( sep_len ) {
        hlt_iterator_bytes sep_end;
        __hlt_bytes_end(&sep_end, sep, excpt, ctx);

        __hlt_bytes_find_bytes(&i, b, sep, excpt, ctx);

        if ( __is_end(i) )
            goto not_found;

        j = i;
        __hlt_iterator_bytes_incr_by(&j, sep_len, excpt, ctx, 0, 0);
    }

    else {
        int looking_for_ws = 1;

        // Special-case: empty separator, split at white-space.
        for ( hlt_bytes* c = b; c && ! __get_object(c); c = c->next ) {
            for ( int8_t* p = c->start; p < c->end; p++ ) {
                if ( looking_for_ws ) {
                    if ( isspace(*p) ) {
                        i.bytes = c;
                        i.cur = p;
                        j = i;
                        __hlt_iterator_bytes_incr(&j, excpt, ctx, 0);
                        looking_for_ws = 0;
                    }
                }

                else {
                    if ( ! isspace(*p) ) {
                        j.bytes = c;
                        j.cur = p;
                        goto done;
                    }
                }
            }
        }

    done:
        if ( looking_for_ws )
            goto not_found;
    }

    hlt_iterator_bytes begin;
    hlt_iterator_bytes end;

    __hlt_bytes_begin(&begin, b, excpt, ctx);
    __hlt_bytes_end(&end, b, excpt, ctx);

    result->first = hlt_bytes_sub(begin, i, excpt, ctx);
    result->second = hlt_bytes_sub(j, end, excpt, ctx);

    return 1;

not_found:
    // Not found, return (b, b"").
    result->first = b;
    result->second = hlt_bytes_new(excpt, ctx);
    return 0;
}

hlt_bytes_pair hlt_bytes_split1(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    hlt_bytes_pair result;
    split1(&result, b, sep, excpt, ctx);
    return result;
}

hlt_vector* hlt_bytes_split(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    hlt_bytes* def = hlt_bytes_new(excpt, ctx);
    hlt_vector* v = hlt_vector_new(&hlt_type_info_hlt_bytes, &def, 0, excpt, ctx);

    while ( 1 ) {
        hlt_bytes_pair result;
        int found = split1(&result, b, sep, excpt, ctx);

        if ( ! found ) {
            hlt_vector_push_back(v, &hlt_type_info_hlt_bytes, &b, excpt, ctx);
            break;
        }

        hlt_vector_push_back(v, &hlt_type_info_hlt_bytes, &result.first, excpt, ctx);
        b = result.second;
    }

    return v;
}

static inline int strip_it(int8_t ch, hlt_bytes* pat)
{
    if ( ! pat )
        return isspace(ch);

    for ( hlt_bytes* c = pat; c && ! __get_object(c); c = c->next ) {
        if ( memchr(c->start, ch, c->end - c->start) )
            return 1;
    }

    return 0;
}

static void _hlt_bytes_strip_calc_iters(hlt_iterator_bytes* start, hlt_iterator_bytes* end,
                                        hlt_bytes* b, hlt_enum side, hlt_bytes* pat,
                                        hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* c = 0;
    int8_t* p = 0;

    if ( hlt_enum_equal(side, Hilti_Side_Left, excpt, ctx) ||
         hlt_enum_equal(side, Hilti_Side_Both, excpt, ctx) ) {
        for ( c = b; c && ! __get_object(c); c = c->next ) {
            for ( p = c->start; p < c->end; p++ ) {
                if ( ! strip_it(*p, pat) )
                    goto end_loop1;
            }
        }

    end_loop1:
        *start = __create_iterator(c, p);
    }

    else
        *start = __create_iterator(b, b->start);

    if ( hlt_enum_equal(side, Hilti_Side_Right, excpt, ctx) ||
         hlt_enum_equal(side, Hilti_Side_Both, excpt, ctx) ) {
        for ( c = __tail(b, false); c && ! __get_object(c); c = __pred(b, c) ) {
            for ( p = c->end - 1; p >= c->start; --p ) {
                if ( ! strip_it(*p, pat) ) {
                    ++p;
                    goto end_loop2;
                }
            }
        }

    end_loop2:
        *end = __create_iterator(c, p);
    }

    else
        *end = GenericEndPos;
}

void hlt_bytes_strip_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* b, hlt_enum side, hlt_bytes* pat,
                             hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes start;
    hlt_iterator_bytes end;

    _hlt_bytes_strip_calc_iters(&start, &end, b, side, pat, excpt, ctx);

    hlt_bytes_sub_hoisted(dst, start, end, excpt, ctx);
}

hlt_bytes* hlt_bytes_strip(hlt_bytes* b, hlt_enum side, hlt_bytes* pat, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    hlt_iterator_bytes start;
    hlt_iterator_bytes end;

    _hlt_bytes_strip_calc_iters(&start, &end, b, side, pat, excpt, ctx);

    return hlt_bytes_sub(start, end, excpt, ctx);
}

static void _hlt_bytes_join_into(hlt_bytes* dst, hlt_bytes* sep, hlt_list* l, hlt_exception** excpt,
                                 hlt_execution_context* ctx)
{
    const hlt_type_info* ti = hlt_list_element_type_from_list(l, excpt, ctx);
    hlt_iterator_list i = hlt_list_begin(l, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(l, excpt, ctx);

    int first = 1;

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {
        if ( ! first )
            hlt_bytes_append(dst, sep, excpt, ctx);

        // FIXME: The charset handling isn't really right here. We should
        // probably change the to_string methods to take a flag indicating
        // "binary is ok", and then have a hlt_bytes_from_object() function
        // that just passes that back. Or maybe we even need a separate
        // to_bytes() function?
        void* obj = hlt_iterator_list_deref(i, excpt, ctx);

        hlt_string so = hlt_object_to_string(ti, obj, 0, excpt, ctx);
        hlt_bytes* bo = hlt_string_encode(so, Hilti_Charset_UTF8, excpt, ctx);
        hlt_bytes_append(dst, bo, excpt, ctx);

        i = hlt_iterator_list_incr(i, excpt, ctx);
        first = 0;
    }
}

void hlt_bytes_join_hoisted(__hlt_bytes_hoisted* dst, hlt_bytes* sep, hlt_list* l,
                            hlt_exception** excpt, hlt_execution_context* ctx)
{
    _hlt_bytes_init_hoisted(dst, 0, 0, ctx);
    _hlt_bytes_join_into(&dst->b, sep, l, excpt, ctx);
}


hlt_bytes* hlt_bytes_join(hlt_bytes* sep, hlt_list* l, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_bytes_new(excpt, ctx);
    _hlt_bytes_join_into(b, sep, l, excpt, ctx);
    return b;
}

void hlt_bytes_append_object(hlt_bytes* b, const hlt_type_info* type, void* obj,
                             hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type);

    if ( ! (b && obj) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    // We make sure that after an object, there's another (emoty) chunk so
    // that we can reliably move iterators past the object.

    hlt_bytes* c1 = _hlt_bytes_new_object(type, obj, ctx);
    hlt_bytes* c2 = _hlt_bytes_new(0, 0, 0, ctx);
    __add_chunk(__tail(b, true), c1, ctx);
    __add_chunk(c1, c2, ctx);

    hlt_thread_mgr_unblock(&b->blockable, ctx);
}

void* hlt_bytes_retrieve_object(hlt_iterator_bytes i, const hlt_type_info* type,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&i);

    if ( ! __at_object(i) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    __hlt_bytes_object* o = __get_object(i.bytes);
    assert(o && o->type);

    if ( type && ! __hlt_type_equal(type, o->type) ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    return &o->object;
}

int8_t hlt_bytes_at_object(hlt_iterator_bytes i, const hlt_type_info* type, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    __normalize_iter(&i);
    return __at_object(i);
}

int8_t hlt_bytes_at_object_of_type(hlt_iterator_bytes i, const hlt_type_info* type,
                                   hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&i);

    __hlt_bytes_object* o = i.bytes ? __get_object(i.bytes) : 0;

    if ( ! o )
        return false;

    return __hlt_type_equal(type, o->type);
}

hlt_iterator_bytes hlt_bytes_skip_object(hlt_iterator_bytes old, hlt_exception** excpt,
                                         hlt_execution_context* ctx)
{
    __normalize_iter(&old);

    if ( ! __at_object(old) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return GenericEndPos;
    }

    // append_object ensures that there's another block to point at.
    assert(old.bytes->next);

    hlt_iterator_bytes ni;
    ni.bytes = old.bytes->next;
    ni.cur = ni.bytes->start;
    return ni;
}

hlt_iterator_bytes hlt_bytes_next_object(hlt_iterator_bytes old, hlt_exception** excpt,
                                         hlt_execution_context* ctx)
{
    __normalize_iter(&old);

    hlt_iterator_bytes ni = old;

    while ( ! __is_end(ni) ) {
        if ( __at_object(ni) )
            break;

        if ( ! ni.bytes->next ) {
            ni.cur = ni.bytes->end;
            break;
        }

        ni.bytes = ni.bytes->next;
        ni.cur = ni.bytes->start;
    }

    return ni;
}

hlt_bytes_size hlt_iterator_bytes_index(hlt_iterator_bytes i, hlt_exception** excpt,
                                        hlt_execution_context* ctx)
{
    __normalize_iter(&i);

    if ( ! i.bytes )
        return -1;

    if ( __at_object(i) )
        return i.bytes->offset;

    return i.bytes->offset + (((i.cur <= i.bytes->end) ? i.cur : i.bytes->end) - i.bytes->start);
}

void hlt_bytes_append_mark(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_bytes_append_mark(b, -1, ctx);
}

hlt_iterator_bytes hlt_bytes_next_mark(hlt_iterator_bytes i, hlt_exception** excpt,
                                       hlt_execution_context* ctx)
{
    __normalize_iter(&i);

    hlt_bytes* b = i.bytes;
    int8_t* c = i.cur + 1; // Skip current position.

    while ( 1 ) {
        if ( __get_object(b) )
            // End of iteration, but not found.
            return __create_iterator(b, b->end);

        for ( hlt_bytes_size* m = b->marks; m && *m != -1; m++ ) {
            if ( *m >= (c - b->start) )
                return __create_iterator(b, b->start + *m);
        }

        if ( ! b->next )
            // No mark found.
            return __create_iterator(b, b->end);

        b = b->next;
        c = b->start;
    }
}

int8_t hlt_bytes_at_mark(hlt_iterator_bytes i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __normalize_iter(&i);
    return __at_mark(i);
}
