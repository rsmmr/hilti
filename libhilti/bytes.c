//
// Note that we manitain two invariants:
//
//  - The head and tail pointers must always point to a chunk. For an empty
//  bytes object, they must point to one with equal start and end addresses.
//
//  - Only that head chunk of a bytes object may have equal start and end
// pointers. If so, that marks and empty bytes object.
//

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "bytes.h"
#include "memory_.h"
#include "exceptions.h"
#include "string_.h"
#include "globals.h"
#include "hutil.h"
#include "threading.h"
#include "int.h"
#include "autogen/hilti-hlt.h"

typedef struct {
    __hlt_gchdr __gchdr; // Header for memory management.
    int8_t *data;        // Data.
} __hlt_bytes_data;

struct __hlt_bytes_chunk {
    __hlt_gchdr __gchdr;      // Header for memory management.
    __hlt_bytes_data* bytes;  // Data block.
    int8_t* start;            // Pointer to first data byte inside block.
    int8_t* end;              // Pointer one beyond the last data byte.
    __hlt_bytes_chunk* next;  // Successor in bytes object.
    __hlt_bytes_chunk* prev;  // Predecessor in bytes object. NOTE: No ref'cnt here to avoid cylces.

    int8_t owner;             // True if we "own" the data, i.e., allocated it.
    int8_t frozen;            // True if the object has been frozen.
    int16_t free;             // Bytes still available in allocated block. Only valid if owner.
};

struct __hlt_bytes {
    __hlt_gchdr __gchdr;     // Header for memory management.
    __hlt_bytes_chunk* head; // First chunk.
    __hlt_bytes_chunk* tail; // Last chunk.  NOTE: No ref'cnt here to avoid cylces.
    __hlt_thread_mgr_blockable blockable; // For blocking until changed.
};

__HLT_RTTI_GC_TYPE(__hlt_bytes_data,  HLT_TYPE_BYTES_DATA)
__HLT_RTTI_GC_TYPE(__hlt_bytes_chunk, HLT_TYPE_BYTES_CHUNK)

void __hlt_bytes_data_dtor(hlt_type_info* ti, __hlt_bytes_data* d)
{
    hlt_free(d->data);
}

void __hlt_bytes_chunk_dtor(hlt_type_info* ti, __hlt_bytes_chunk* c)
{
    GC_CLEAR(c->bytes, __hlt_bytes_data);
    GC_CLEAR(c->next, __hlt_bytes_chunk);
    c->prev = 0;
}

void hlt_bytes_dtor(hlt_type_info* ti, hlt_bytes* b)
{
    GC_CLEAR(b->head, __hlt_bytes_chunk);
    b->tail = 0;
}

void hlt_iterator_bytes_dtor(hlt_type_info* ti, hlt_iterator_bytes* p)
{
    if ( ! p->chunk )
        return;

    GC_CLEAR(p->chunk, __hlt_bytes_chunk);
}

void hlt_iterator_bytes_cctor(hlt_type_info* ti, hlt_iterator_bytes* p)
{
    if ( ! p->chunk )
        return;

    GC_CCTOR(p->chunk, __hlt_bytes_chunk)
}

// If a chunk of data is of size is less than this, we can decide to copy it
// into space we allocated ourselves.
static const int16_t MAX_COPY_SIZE = 256;

// If we allocate a chunk of a data ourselves, it will be of this size.
static const int16_t ALLOC_SIZE = 1024;

// Sentinial for marking a chunk as empty by having both start and end point
// here.
static int8_t empty_sentinel;

static inline int8_t is_empty_chunk(const __hlt_bytes_chunk* chunk)
{
    return (chunk->start == chunk->end);
}

static inline int8_t is_empty(const hlt_bytes* b)
{
    assert(b->head);
    return is_empty_chunk(b->head);
}

static inline int8_t is_end(hlt_iterator_bytes pos)
{
    return pos.chunk == 0 || (!pos.cur) || pos.cur >= pos.chunk->end;
}

static inline void normalize_pos(hlt_iterator_bytes* pos, int adj_refcnt)
{
    if ( ! pos->chunk )
        return;

    // If the pos was previously an end position but now new data has been
    // added, adjust it so that it's pointing to the next byte.
    if ( (! pos->cur || pos->cur >= pos->chunk->end) && pos->chunk->next ) {

        // The ref cnt must only be adjusted if the HILTI layer will see the
        // normalized iterator.
        if ( adj_refcnt ) {
            GC_ASSIGN(pos->chunk, pos->chunk->next, __hlt_bytes_chunk);
        }
        else
            pos->chunk = pos->chunk->next;

        pos->cur = pos->chunk->start;
    }

}

void __bytes_normalize_pos(hlt_iterator_bytes* pos, int adj_refcnt)
{
    return normalize_pos(pos, adj_refcnt);
}

// For debugging.
static void __print_bytes(const char* prefix, const hlt_bytes* b)
{
    fprintf(stderr, "%s: %p b( ", prefix, b);
    for ( __hlt_bytes_chunk *c = b->head; c; c = c->next )
        fprintf(stderr, "#%ld:%p-%p(%d) ", c->end - c->start, c->start, c->end, c->free);
    fprintf(stderr, ")\n");
}

// c already ref'ed.
static void add_chunk(hlt_bytes* b, __hlt_bytes_chunk* c)
{
    assert(b);
    assert(c);

    if ( is_empty_chunk(c) ) {
        GC_CLEAR(c, __hlt_bytes_chunk);
        return;
    }

    GC_CLEAR(c->next, __hlt_bytes_chunk);

    if ( b->tail ) {
        assert(b->head);
        assert(!b->tail->next);
        c->prev = b->tail;
        b->tail->next = c; // Consumes ref cnt.
        b->tail = c; // Not ref cnt.
    }

    else {
        assert( ! b->head );
        c->prev = 0;
        b->head = c; // Consumes ref cnt.
        b->tail = c; // Not ref cnt.
    }

    // We added data, so the first chunk must not be empty anymore because
    // that would signal an empty bytes object.
    if ( is_empty_chunk(b->head) ) {
        // Remove first chunk (cannot be tail at this point).
        assert(b->head != b->tail);
        b->head->next->prev = 0;
        GC_ASSIGN(b->head, b->head->next, __hlt_bytes_chunk);
    }

    //assert(!is_empty_chunk(b->head));
}

hlt_bytes* hlt_bytes_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = GC_NEW(hlt_bytes);

    __hlt_bytes_chunk* dst = GC_NEW(__hlt_bytes_chunk);
    GC_ASSIGN(b->head, dst, __hlt_bytes_chunk);
    b->tail = dst;

    GC_CLEAR(dst->bytes, __hlt_bytes_data);
    dst->start = &empty_sentinel;
    dst->end = &empty_sentinel;

    GC_CLEAR(dst, __hlt_bytes_chunk);

    hlt_thread_mgr_blockable_init(&b->blockable);

    assert(is_empty(b));

    return b;
}

hlt_bytes* hlt_bytes_new_from_data(int8_t* data, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_bytes_new(excpt, ctx);

    if ( len > 0 ) {
        b->head->bytes = GC_NEW(__hlt_bytes_data);
        b->head->bytes->data = data;
        b->head->start = data;
        b->head->end = data + len;
        assert(!is_empty(b));
    }

    else
        hlt_free(data);

    return b;
}

hlt_bytes* hlt_bytes_new_from_data_copy(const int8_t* data, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_bytes_new(excpt, ctx);

    if ( len > 0 ) {
        int8_t* copy = hlt_malloc(len);
        memcpy(copy, data, len);
        b->head->bytes = GC_NEW(__hlt_bytes_data);
        b->head->bytes->data = copy;
        b->head->start = copy;
        b->head->end = copy + len;
        assert(!is_empty(b));
    }

    return b;
}

// Returns the number of bytes stored.
hlt_bytes_size hlt_bytes_len(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    hlt_bytes_size len = 0;

    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next )
        len += c->end - c->start;

    return len;
}

// Returns true if empty.
int8_t hlt_bytes_empty(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    return is_empty(b);
}

// Returns true if we're using the raw data passed in, and false if we've
// made a copy. In the latter case, the raw data will have been freed if owner is true.
int8_t __hlt_bytes_append_raw(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx, int8_t owner)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);

        if ( owner )
            hlt_free(raw);

        return 0;
    }

    if ( hlt_bytes_is_frozen(b, excpt, ctx) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);

        if ( owner )
            hlt_free(raw);

        return 0;
    }

    if ( ! len ) {
        // Empty.
        if ( owner )
            hlt_free(raw);

        return 0;
    }

    hlt_thread_mgr_unblock(&b->blockable, ctx);

    // Special case: see if we can copy it into the last chunk.
    if ( b->tail && b->tail->owner && b->tail->free >= len ) {
        memcpy((int8_t*)b->tail->end, raw, len);
        b->tail->end += len;
        b->tail->free -= len;

        if ( owner )
            hlt_free(raw);

        return 0;
    }

    __hlt_bytes_chunk* dst = GC_NEW(__hlt_bytes_chunk);
    dst->bytes = GC_NEW(__hlt_bytes_data);

    // If the raw data is small, we copy it over into a newly allocated block
    // to avoid fragmentation.
    if ( len <= MAX_COPY_SIZE ) {
        int8_t* mem = hlt_malloc(ALLOC_SIZE);
        dst->bytes->data = mem;
        dst->start = mem;
        dst->end = mem + len;
        dst->owner = 1;
        dst->frozen = 0;
        dst->free = ALLOC_SIZE - (dst->end - dst->start);
        memcpy(mem, raw, len);
        add_chunk(b, dst);

        if ( owner )
            hlt_free(raw);

        return 0;
    }

    else {
        int8_t* data = raw;

        if ( ! owner ) {
            data = hlt_malloc(len);
            memcpy(data, raw, len);
        }

        dst->bytes->data = data;
        dst->start = data;
        dst->end = data + len;
        dst->owner = 0;
        dst->frozen = 0;
        add_chunk(b, dst);
        return 1;
    }
}

// Appends one Bytes object to another.
void hlt_bytes_append(hlt_bytes* b, const hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    if ( b == other || hlt_bytes_is_frozen(b, excpt, ctx) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    if ( is_empty(other) )
        // Empty.
        return;

    hlt_thread_mgr_unblock(&b->blockable, ctx);

#if 0
    /// FIXME: This isn't working I believe.

    // Special case: if the other object has only one chunk, pass it on to
    // the *_raw version which might decide to copy the data over.
    if ( other->head == other->tail ) {
        int need_cctor = __hlt_bytes_append_raw(b, other->head->start, other->head->end - other->head->start, excpt, ctx, 0);

        if ( need_cctor )
            GC_CCTOR(other->head->bytes, __hlt_bytes_data);

        return;
    }
#endif

    // XX Leak triggered by code below.

    for ( const __hlt_bytes_chunk* src = other->head; src; src = src->next ) {
        __hlt_bytes_chunk* dst = GC_NEW(__hlt_bytes_chunk);
        GC_ASSIGN(dst->bytes, src->bytes, __hlt_bytes_data);
        dst->start = src->start;
        dst->end = src->end;
        dst->owner = 0;
        dst->frozen = 0;
        add_chunk(b, dst);
    }
}

void hlt_bytes_append_raw(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_bytes_append_raw(b, raw, len, excpt, ctx, 1);
}

void hlt_bytes_append_raw_copy(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_bytes_append_raw(b, raw, len, excpt, ctx, 0);
}

hlt_bytes* hlt_bytes_concat(hlt_bytes* b1, const hlt_bytes* b2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* s = hlt_bytes_copy(b1, excpt, ctx);
    hlt_bytes_append(s, b2, excpt, ctx);
    return s;
}

static hlt_iterator_bytes GenericEndPos = { 0, 0 };

hlt_iterator_bytes hlt_bytes_find_byte(hlt_bytes* b, int8_t chr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    for ( __hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        int8_t* p = memchr(c->start, chr, c->end - c->start);
        if ( p ) {
            hlt_iterator_bytes i = { c, p };
            GC_CCTOR(c, __hlt_bytes_chunk);
            return i;
        }
    }

    return GenericEndPos;
}

int8_t hlt_bytes_contains_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes i = hlt_bytes_find_bytes(b, other, excpt, ctx);
    int8_t result = (! is_end(i));
    GC_DTOR(i, hlt_iterator_bytes);
    return result;
}

hlt_iterator_bytes hlt_bytes_find_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes i = hlt_bytes_begin(other, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(other, excpt, ctx);

    if ( hlt_iterator_bytes_eq(i, end, excpt, ctx) ) {
        // Empty pattern returns start position.
        GC_DTOR(i, hlt_iterator_bytes);
        GC_DTOR(end, hlt_iterator_bytes);
        return hlt_bytes_begin(b, excpt, ctx);
    }

    assert(i.cur);
    hlt_iterator_bytes p = hlt_bytes_find_byte(b, *i.cur, excpt, ctx);

    GC_DTOR(i, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    end = hlt_bytes_end(b, excpt, ctx);

    if ( hlt_iterator_bytes_eq(p, end, excpt, ctx) ) {
        GC_DTOR(end, hlt_iterator_bytes);
        return p; // Not found.
    }

    GC_DTOR(end, hlt_iterator_bytes);

    if ( hlt_bytes_match_at(p, other, excpt, ctx) )
        // Found.
        return p;

    // Not found.
    GC_DTOR(p, hlt_iterator_bytes);
    return GenericEndPos;
}

hlt_bytes* hlt_bytes_copy(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes begin = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    hlt_bytes* s = hlt_bytes_sub(begin, end, excpt, ctx);

    GC_DTOR(begin, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);
    return s;
}

hlt_bytes* hlt_bytes_sub(hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start, 0);
    normalize_pos(&end, 0);

    if ( hlt_iterator_bytes_eq(start, end, excpt, ctx) )
        // Return an empty bytes object.
        return hlt_bytes_new(excpt, ctx);

    if ( is_end(start) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    hlt_bytes* b = hlt_bytes_new(excpt, ctx);

    // Special case: if both positions are inside the same chunk, it's easy.
    if ( start.chunk == end.chunk || (is_end(end) && start.chunk->next == 0) ) {
        __hlt_bytes_chunk* c = GC_NEW(__hlt_bytes_chunk);
        GC_ASSIGN(c->bytes, start.chunk->bytes, __hlt_bytes_data);
        c->start = start.cur;
        c->end = end.cur ? end.cur : start.chunk->end;
        c->owner = 0;
        c->frozen = 0;
        add_chunk(b, c);
        return b;
    }

    // Create the first chunk of the sub-bytes.
    __hlt_bytes_chunk* first = GC_NEW(__hlt_bytes_chunk);
    GC_ASSIGN(first->bytes, start.chunk->bytes, __hlt_bytes_data);
    first->start = start.cur;
    first->end = start.chunk->end;
    first->owner = 0;
    first->frozen = 0;
    add_chunk(b, first);

    // Copy the chunks in between.
    for ( const __hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {

        if ( ! c ) {
            if ( is_end(end) )
                break;

            // Start and end are not part of the same Bytes object!
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        __hlt_bytes_chunk* dst = GC_NEW(__hlt_bytes_chunk);
        GC_ASSIGN(dst->bytes, c->bytes, __hlt_bytes_data);
        dst->start = c->start;
        dst->end = c->end;
        dst->owner = 0;
        dst->frozen = 0;
        add_chunk(b, dst);
    }

    if ( ! is_end(end) ) {
        // Create the last chunk of the sub-bytes.
        __hlt_bytes_chunk* last = GC_NEW(__hlt_bytes_chunk);
        GC_ASSIGN(last->bytes, end.chunk->bytes, __hlt_bytes_data);
        last->start = end.chunk->start;
        last->end = end.cur;
        last->owner = 0;
        last->frozen = 0;
        add_chunk(b, last);
    }

    return b;
}

static int8_t* hlt_bytes_sub_raw_internal(hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start, 0);
    normalize_pos(&end, 0);

    hlt_bytes_size len = hlt_iterator_bytes_diff(start, end, excpt, ctx);

    if ( len == 0 ) {
        // Can't happen because we should have be in the easy case.
        hlt_set_exception(excpt, &hlt_exception_internal_error, 0);
        return 0;
        }

    if ( len < 0 ) {
        // Wrong order of arguments.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    // The potentially expensive case: we need to copy the data into a
    // continous memory region.
    int8_t* mem = hlt_malloc(len);
    int8_t* p = mem;

    // Copy the first chunk.
    hlt_bytes_size n = start.chunk->end - start.cur;
    memcpy(p, start.cur, n);
    p += n;

    // Copy the chunks in between.
    for ( const __hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {

        if ( ! c ) {

            if ( is_end(end) )
                break;

            // Start and end are not part of the same Bytes object!
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        n = c->end - c->start;
        memcpy(p, c->start, n);
        p += n;
    }

    if ( ! is_end(end) ) {
        // Copy the last chunk.
        n = end.cur - end.chunk->start;
        memcpy(p, end.chunk->start, n);
        p += n;
        assert(p - mem == len);
    }

    return mem;
}

int8_t hlt_bytes_match_at(hlt_iterator_bytes pos, hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos, 0);

    if ( is_end(pos) )
        return hlt_bytes_len(b, excpt, ctx) == 0;

    if ( hlt_bytes_len(b, excpt, ctx) == 0 )
        return 0;

    // No need for ref'cnt, we don't keep them.
    hlt_iterator_bytes c1 = pos;
    hlt_iterator_bytes c2 = { b->head, b->head->start };

    while ( 1 ) {
        // Extract bytes.
        int8_t b1 = *(c1.cur);
        int8_t b2 = *(c2.cur);

        if ( b1 != b2 )
            return 0;

        // Increase iterator 1.

        int8_t end1 = 0;
        int8_t end2 = 0;

        if ( c1.cur < c1.chunk->end - 1 )
            ++c1.cur;

        else {
            if ( c1.chunk->next ) {
                c1.chunk = c1.chunk->next;
                c1.cur = c1.chunk->start;
            }
            else {
                c1.cur = c1.chunk->end;
                end1 = 1;
            }
        }

        // Increase iterator 2.

        if ( c2.cur < c2.chunk->end - 1 )
            ++c2.cur;

        else {
            if ( c2.chunk->next ) {
                c2.chunk = c2.chunk->next;
                c2.cur = c2.chunk->start;
            }
            else {
                c2.cur = c2.chunk->end;
                end2 = 1;
            }
        }

        // End of pattern reached?
        if ( end2 )
            // Found.
            return 1;

        // End of input reached?
        if ( end1 )
            // Not found.
            return 0;
    }

    // Can't be reached.
    assert(0);
}

int8_t* hlt_bytes_sub_raw(hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start, 0);
    normalize_pos(&end, 0);

    if ( is_end(start) ) {
        if ( ! is_end(end) )
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);

        return 0;
    }

    // The easy case: start and end are within the same chunk.
    if ( start.chunk == end.chunk ) {
        size_t len = start.chunk->end - start.cur;
        int8_t* mem = hlt_malloc(len);
        memcpy(mem, start.cur, len);
        return mem;
    }

    // We split the rest into its own function to make it easier for the
    // compiler to inline the fast case.
    return hlt_bytes_sub_raw_internal(start, end, excpt, ctx);
}

int8_t* hlt_bytes_to_raw(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    hlt_iterator_bytes begin = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    int8_t* i = hlt_bytes_sub_raw(begin, end, excpt, ctx);

    GC_DTOR(begin, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return i;
}

int8_t* hlt_bytes_to_raw_buffer(const hlt_bytes* b, int8_t* buffer, hlt_bytes_size buffer_size, hlt_exception** excpt, hlt_execution_context* ctx)
{
    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        hlt_bytes_size chunk_size = (c->end - c->start);
        hlt_bytes_size n = (chunk_size > buffer_size ? buffer_size : chunk_size);
        memcpy(buffer, c->start, n);

        if ( chunk_size > buffer_size )
            return 0;

        buffer += n;
        buffer_size -= n;
    }

    return buffer;
}

int8_t __hlt_bytes_extract_one(hlt_iterator_bytes* pos, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(pos, 1);
    normalize_pos(&end, 0);

    if ( is_end(*pos) || hlt_iterator_bytes_eq(*pos, end, excpt, ctx) ) {
        GC_DTOR(*pos, hlt_iterator_bytes);
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        return 0;
    }

    // Extract byte.
    assert(pos->cur);
    int8_t b = *(pos->cur);

    // Increase iterator.
    assert(pos->chunk);
    if ( pos->cur < pos->chunk->end - 1 )
        // We stay inside chunk.
        ++pos->cur;

    else {
        if ( pos->chunk->next ) {
            // Switch to next chunk.
            GC_ASSIGN(pos->chunk, pos->chunk->next, __hlt_bytes_chunk);
            pos->cur = pos->chunk->start;
        }
        else {
            // End reached.
            pos->cur = pos->chunk->end;
        }
    }

    return b;
}

hlt_iterator_bytes hlt_bytes_offset(const hlt_bytes* b, hlt_bytes_size pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return GenericEndPos;
    }

    if ( pos < 0 )
        pos += hlt_bytes_len(b, excpt, ctx);

    if ( pos < 0 )
        return hlt_bytes_end(b, excpt, ctx);

    __hlt_bytes_chunk* c;
    for ( c = b->head; c && pos >= (c->end - c->start); c = c->next ) {
        pos -= (c->end - c->start);
    }

    if ( ! c ) {
        // Position is out range, return end().
        return hlt_bytes_end(b, excpt, ctx);
    }

    hlt_iterator_bytes p;
    GC_INIT(p.chunk, c, __hlt_bytes_chunk);
    p.cur = c->start + pos;
    return p;
}

hlt_iterator_bytes hlt_bytes_begin(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return GenericEndPos;
    }

    if ( hlt_bytes_len(b, excpt, ctx) == 0 )
        return hlt_bytes_end(b, excpt, ctx);

    hlt_iterator_bytes p;
    GC_INIT(p.chunk, b->head, __hlt_bytes_chunk);
    p.cur = b->head ? b->head->start : 0;

    return p;
}

hlt_iterator_bytes hlt_bytes_end(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return GenericEndPos;
    }

    hlt_iterator_bytes p;
    GC_INIT(p.chunk, b->tail, __hlt_bytes_chunk);
    p.cur = b->tail ? b->tail->end : 0;
    return p;
}

hlt_iterator_bytes hlt_bytes_generic_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GenericEndPos;
}

void hlt_bytes_freeze(hlt_bytes* b, int8_t freeze, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    if ( (! b->tail) && freeze ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    assert(b->tail);
    b->tail->frozen = freeze;
    hlt_thread_mgr_unblock(&b->blockable, ctx);
}

void hlt_bytes_trim(hlt_bytes* b, hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos, 0);

    if ( ! (pos.chunk && pos.cur) )
        return;

    GC_ASSIGN(b->head, pos.chunk, __hlt_bytes_chunk);
    GC_ASSIGN(b->head->bytes, pos.chunk->bytes, __hlt_bytes_data);
    b->head->prev = 0;
    b->head->start = pos.cur;
}

int8_t hlt_bytes_is_frozen(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    return b->tail ? b->tail->frozen : 0;
}

hlt_iterator_bytes hlt_iterator_bytes_copy(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes copy;
    GC_INIT(copy.chunk, pos.chunk, __hlt_bytes_chunk);
    copy.cur = pos.cur;
    normalize_pos(&copy, 1);
    return copy;
}

int8_t hlt_iterator_bytes_is_frozen(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos, 0);

    if ( ! pos.chunk )
        return 0;

    // Go to last chunk of string.
    const __hlt_bytes_chunk* c;
    for ( c = pos.chunk; c->next; c = c->next );

    return c->frozen;
}

int8_t hlt_iterator_bytes_deref(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos, 0);

    if ( is_end(pos) ) {
        // Position is out range.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    return *pos.cur;
}

hlt_iterator_bytes hlt_iterator_bytes_incr(hlt_iterator_bytes old, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes pos = hlt_iterator_bytes_copy(old, excpt, ctx);

    if ( is_end(pos) )
        // Fail silently.
        return pos;

    // Can we stay inside the same chunk?
    if ( pos.cur < pos.chunk->end - 1 ) {
        ++pos.cur;
        return pos;
    }

    if ( pos.chunk->next ) {
        // Need to switch chunk.
        GC_ASSIGN(pos.chunk, pos.chunk->next, __hlt_bytes_chunk);
        pos.cur = pos.chunk->start;
    }
    else
        // End reached.
        pos.cur = pos.chunk->end;

    return pos;
}

hlt_iterator_bytes hlt_iterator_bytes_incr_by(hlt_iterator_bytes old, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes pos = hlt_iterator_bytes_copy(old, excpt, ctx);

    if ( ! n )
        return pos;

    if ( is_end(pos) )
        // Fail silently.
        return pos;

    while ( 1 ) {
        // Can we stay inside the same chunk?
        if ( pos.cur + n < pos.chunk->end ) {
            pos.cur += n;
            return pos;
        }

        n -= pos.chunk->end - pos.cur;

        if ( pos.chunk->next ) {
            // Need to switch chunk.
            GC_ASSIGN(pos.chunk, pos.chunk->next, __hlt_bytes_chunk);
            pos.cur = pos.chunk->start;
        }
        else {
            // End reached.
            pos.cur = pos.chunk->end;
            return pos;
        }
    }
}


#if 0
// This is a bit tricky because pos_end() doesn't return anything suitable
// for iteration.  Let's leave this out for now until we know whether we
// actually need reverse iteration.
void hlt_iterator_bytes_decr(hlt_iterator_bytes* pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos);

    if ( is_end(pos) )
        // Fail silently.
        return;

    // Can we stay inside the same chunk?
    if ( pos->cur > pos->chunk->start ) {
        --pos->cur;
        return;
    }

    // Need to switch chunk.
    pos->chunk = pos->chunk->prev;
    if ( ! pos->chunk ) {
        // End reached.
        *pos = GenericEndPos; XXX not correct anymore XXX
        return;
    }

    pos->cur = pos->chunk->end - 1;
    pos->end = pos->chunk->end;
    return;
}
#endif

int8_t hlt_iterator_bytes_eq(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos1, 0);
    normalize_pos(&pos2, 0);

    if ( is_end(pos1) && is_end(pos2) )
        return 1;

    return pos1.cur == pos2.cur && pos1.chunk == pos2.chunk;
}

// Returns the number of bytes from pos1 to pos2 (not counting pos2).
hlt_bytes_size hlt_iterator_bytes_diff(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos1, 0);
    normalize_pos(&pos2, 0);

    if ( hlt_iterator_bytes_eq(pos1, pos2, excpt, ctx) )
        return 0;

    if ( is_end(pos1) && ! is_end(pos2) ) {
        // Invalid starting position.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    // Special case: both inside same chunk.
    if ( pos1.chunk == pos2.chunk ) {
        if ( pos1.cur > pos2.cur ) {
            // Invalid starting position.
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }
        return pos2.cur - pos1.cur;
    }

    // Count first chunk.
    assert(pos1.chunk);
    hlt_bytes_size n = pos1.chunk->end - pos1.cur;

    // Count intermediary chunks.
    const __hlt_bytes_chunk* c;
    for ( c = pos1.chunk->next; is_end(pos2) || c != pos2.chunk; c = c->next ) {
        if ( ! c ) {
            if ( is_end(pos2) )
                break;

            // The two positions are not part of the same Bytes object!
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        n += c->end - c->start;
    }

    if ( c )
        // Count last chunk.
        n += pos2.cur - pos2.chunk->start;

    return n;
}

hlt_string hlt_bytes_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string dst = 0;

    const hlt_bytes* b = *((const hlt_bytes**)obj);

    if ( ! b )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    char hex[5] = { '\\', 'x', 'X', 'X', '\0' };
    char buffer[256];
    uint64_t i = 0;

    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        for ( const int8_t* p = c->start; p < c->end; ++p ) {

            if ( i > sizeof(buffer) - 10 ) {
                hlt_string tmp1 = dst;
                hlt_string tmp2 = hlt_string_from_data((int8_t*)buffer, i, excpt, ctx);
                dst = hlt_string_concat(tmp1, tmp2, excpt, ctx);
                GC_DTOR(tmp1, hlt_string);
                GC_DTOR(tmp2, hlt_string);
                i = 0;
            }

            unsigned char c = *(unsigned char *)p;

            if ( isprint((char)c) && c < 128 )
                buffer[i++] = c;

            else {
                hlt_util_uitoa_n(c, hex + 2, 3, 16, 1);
                strcpy(buffer + i, hex);
                i += sizeof(hex) - 1;
            }
        }
    }

    if ( i ) {
        hlt_string tmp1 = dst;
        hlt_string tmp2 = hlt_string_from_data((int8_t*)buffer, i, excpt, ctx);
        dst = hlt_string_concat(tmp1, tmp2, excpt, ctx);
        GC_DTOR(tmp1, hlt_string);
        GC_DTOR(tmp2, hlt_string);
    }

    return dst;
}

hlt_hash hlt_bytes_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = *((hlt_bytes**)obj);

    hlt_hash hash = 0;

    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next )
        hash += hlt_hash_bytes(c->start, c->end - c->start);

    return hash;
}

int8_t hlt_bytes_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b1 = *((hlt_bytes**)obj1);
    hlt_bytes* b2 = *((hlt_bytes**)obj2);

    hlt_execution_context* c= hlt_global_execution_context();
    hlt_exception* e = 0;

    return hlt_bytes_cmp(b1, b2, &e, c) == 0;
}

void* hlt_bytes_blockable(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = *((hlt_bytes**)obj);
    return &b->blockable;
}

void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start, 0);
    normalize_pos(&end, 0);

    if ( ! cookie ) {

        if ( hlt_iterator_bytes_eq(start, end, excpt, ctx) ) {
            block->start = block->end = start.cur;
            block->next = 0;
            return 0;
        }

        if ( is_end(start) && ! is_end(end) ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        if ( start.chunk != end.chunk  ) {
            assert(start.chunk);
            block->start = start.cur;
            block->end = start.chunk->end;
            block->next = start.chunk->next;
        }
        else {
            block->start = start.cur;
            block->end = end.cur;
            block->next = 0;
            }
    }

    else {

        if ( ! block->next )
            return 0;

        if ( block->next != end.chunk ) {
            block->start = block->next->start;
            block->end = block->next->end;
            block->next = block->next->next;
        }
        else {
            block->start = end.chunk->start;
            block->end = end.cur;
            block->next = 0;
        }
    }

    // We don't care what we return if we haven't reached the end yet as long
    // as it's not NULL.
    return block->next ? block : 0;
}

int8_t hlt_bytes_cmp(const hlt_bytes* b1, const hlt_bytes* b2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! (b1 && b2) ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    // FIXME: This is not the smartest implementation. Rather than using the
    // iterator functions, we should compare larger chunks at once.
    hlt_iterator_bytes p1 = hlt_bytes_begin(b1, excpt, ctx);
    hlt_iterator_bytes p2 = hlt_bytes_begin(b2, excpt, ctx);

    int8_t result = 0;

    while ( 1 ) {

        if ( is_end(p1) ) {
            result = is_end(p2) ? 0 : 1;
            goto done;
        }

        if ( is_end(p2) ) {
            result = -1;
            goto done;
        }

        int8_t c1 = hlt_iterator_bytes_deref(p1, excpt, ctx);
        int8_t c2 = hlt_iterator_bytes_deref(p2, excpt, ctx);

        if ( c1 != c2 ) {
            result = c1 > c2 ? -1 : 1;
            goto done;
        }

        hlt_iterator_bytes np1 = hlt_iterator_bytes_incr(p1, excpt, ctx);
        hlt_iterator_bytes np2 = hlt_iterator_bytes_incr(p2, excpt, ctx);

        GC_DTOR(p1, hlt_iterator_bytes);
        GC_DTOR(p2, hlt_iterator_bytes);

        p1 = np1;
        p2 = np2;
    }

done:
    GC_DTOR(p1, hlt_iterator_bytes);
    GC_DTOR(p2, hlt_iterator_bytes);

    return result;
}

int64_t hlt_bytes_to_int(hlt_bytes* b, int64_t base, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    int64_t value = 0;
    int64_t t = 0;
    int8_t first = 1;
    int8_t negate = 0;

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {
        int8_t ch = __hlt_bytes_extract_one(&cur, end, excpt, ctx);

        if ( isdigit(ch) )
            t = ch - '0';
        else if ( isalpha(ch) )
            t = 10 + tolower(ch) - 'a';

        else if ( first && ch == '-' )
            negate = 1;

        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            GC_DTOR(cur, hlt_iterator_bytes);
            GC_DTOR(end, hlt_iterator_bytes);
            return 0;
        }

        if ( t >= base ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        value = (value * base) + t;
        first = 0;
    }

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return negate ? -value : value;
}

int64_t hlt_bytes_to_int_binary(hlt_bytes* b, hlt_enum byte_order, hlt_exception** excpt, hlt_execution_context* ctx)
{
    size_t len = hlt_bytes_len(b, excpt, ctx);

    if ( len > 8 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    int64_t i = 0;

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {
        uint8_t ch = __hlt_bytes_extract_one(&cur, end, excpt, ctx);
        i = (i << 8) | ch;
        }

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Little, excpt, ctx) )
        i = hlt_int_flip(i, len, excpt, ctx);

    return i;
}

hlt_bytes* hlt_bytes_lower(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    int8_t* tmp = hlt_malloc(len);
    int8_t* p = tmp;

    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) )
        *p++ = tolower(__hlt_bytes_extract_one(&cur, end, excpt, ctx));

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return hlt_bytes_new_from_data(tmp, len, excpt, ctx);
}

hlt_bytes* hlt_bytes_upper(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    int8_t* tmp = hlt_malloc(len);
    int8_t* p = tmp;

    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) )
        *p++ = toupper(__hlt_bytes_extract_one(&cur, end, excpt, ctx));

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return hlt_bytes_new_from_data(tmp, len, excpt, ctx);
}

int8_t hlt_bytes_starts_with(hlt_bytes* b, hlt_bytes* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len_b = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes_size len_s = hlt_bytes_len(s, excpt, ctx);

    if ( len_s > len_b )
        return 0;

    hlt_iterator_bytes cur_b = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes cur_s = hlt_bytes_begin(s, excpt, ctx);
    hlt_iterator_bytes end_b = hlt_bytes_end(b, excpt, ctx);
    hlt_iterator_bytes end_s = hlt_bytes_end(s, excpt, ctx);

    int result = 1;

    while ( ! hlt_iterator_bytes_eq(cur_s, end_s, excpt, ctx) ) {
        int8_t c1 = __hlt_bytes_extract_one(&cur_b, end_b, excpt, ctx);
        int8_t c2 = __hlt_bytes_extract_one(&cur_s, end_s, excpt, ctx);
        if ( c1 != c2 ) {
            result = 0;
            break;
        }
    }

    GC_DTOR(cur_b, hlt_iterator_bytes);
    GC_DTOR(cur_s, hlt_iterator_bytes);
    GC_DTOR(end_b, hlt_iterator_bytes);
    GC_DTOR(end_s, hlt_iterator_bytes);

    return result;
}

static int8_t split1(hlt_bytes_pair* result, hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size sep_len = hlt_bytes_len(sep, excpt, ctx);

    hlt_iterator_bytes i;
    hlt_iterator_bytes j;

    if ( sep_len ) {
        hlt_iterator_bytes sep_end = hlt_bytes_end(sep, excpt, ctx);

        i = hlt_bytes_find_bytes(b, sep, excpt, ctx);

        if ( hlt_iterator_bytes_eq(i, sep_end, excpt, ctx) ) {
            GC_DTOR(i, hlt_iterator_bytes);
            GC_DTOR(sep_end, hlt_iterator_bytes);
            goto not_found;
        }

        j = hlt_iterator_bytes_incr_by(i, sep_len, excpt, ctx);

        GC_DTOR(sep_end, hlt_iterator_bytes);
    }

    else {
        int looking_for_ws = 1;

        // Special-case: empty separator, split at white-space.
        for ( __hlt_bytes_chunk* c = b->head; c; c = c->next ) {
            for ( int8_t* p = c->start; p < c->end; p++ ) {
                if ( looking_for_ws ) {
                    if ( isspace(*p) ) {
                        i.chunk = c;
                        i.cur = p;
                        GC_CCTOR(i, hlt_iterator_bytes);
                        j = hlt_iterator_bytes_incr(i, excpt, ctx);
                        looking_for_ws = 0;
                    }
                }

                else {
                    if ( ! isspace(*p) ) {
                        GC_DTOR(j, hlt_iterator_bytes);
                        j.chunk = c;
                        j.cur = p;
                        GC_CCTOR(j, hlt_iterator_bytes);
                        goto done;
                    }
                }
            }
        }

done:
        if ( looking_for_ws )
            goto not_found;
    }

    hlt_iterator_bytes begin = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end   = hlt_bytes_end(b, excpt, ctx);

    result->first = hlt_bytes_sub(begin, i, excpt, ctx);
    result->second = hlt_bytes_sub(j, end, excpt, ctx);

    GC_DTOR(begin, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);
    GC_DTOR(i, hlt_iterator_bytes);
    GC_DTOR(j, hlt_iterator_bytes);

    return 1;

not_found:
    // Not found, return (b, b"").
    GC_CCTOR(b, hlt_bytes);
    result->first = b;
    result->second = hlt_bytes_new(excpt, ctx);
    return 0;
}

hlt_bytes_pair hlt_bytes_split1(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_pair result;
    split1(&result, b, sep, excpt, ctx);
    return result;
}

hlt_vector* hlt_bytes_split(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* def = hlt_bytes_new(excpt, ctx);
    hlt_vector* v = hlt_vector_new(&hlt_type_info_hlt_bytes, &def, 0, excpt, ctx);
    GC_DTOR(def, hlt_bytes);

    GC_CCTOR(b, hlt_bytes);

    while ( 1 ) {
        hlt_bytes_pair result;
        int found = split1(&result, b, sep, excpt, ctx);

        if ( ! found ) {
            GC_DTOR(result.first, hlt_bytes);
            GC_DTOR(result.second, hlt_bytes);
            hlt_vector_push_back(v, &hlt_type_info_hlt_bytes, &b, excpt, ctx);
            break;
        }

        hlt_vector_push_back(v, &hlt_type_info_hlt_bytes, &result.first, excpt, ctx);
        GC_DTOR(result.first, hlt_bytes);

        GC_DTOR(b, hlt_bytes);
        b = result.second;
    }

    GC_DTOR(b, hlt_bytes);

    return v;
}

static inline int strip_it(int8_t ch, hlt_bytes* pat)
{
    if ( ! pat )
        return isspace(ch);

    for ( __hlt_bytes_chunk* c = pat->head; c; c = c->next ) {
        if ( memchr(c->start, ch, c->end - c->start) )
            return 1;
    }

    return 0;
}

hlt_bytes* hlt_bytes_strip(hlt_bytes* b, hlt_enum side, hlt_bytes* pat, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes start;
    hlt_iterator_bytes end;

    __hlt_bytes_chunk* c = 0;
    int8_t* p = 0;

    if ( hlt_enum_equal(side, Hilti_Side_Left, excpt, ctx) ||
         hlt_enum_equal(side, Hilti_Side_Both, excpt, ctx) ) {

        for ( c = b->head; c; c = c->next ) {
            for ( p = c->start; p < c->end; p++ ) {
                if ( ! strip_it(*p, pat) )
                    goto end_loop1;
            }
        }

end_loop1:
    start.chunk = c;
    start.cur = p;
    normalize_pos(&start, 0);
    }

    else {
        start.chunk = b->head;
        start.cur = b->head ? b->head->start : 0;
    }

    if ( hlt_enum_equal(side, Hilti_Side_Right, excpt, ctx) ||
         hlt_enum_equal(side, Hilti_Side_Both, excpt, ctx) ) {

        for ( c = b->tail; c; c = c->prev ) {
            for ( p = c->end - 1; p >= c->start; --p ) {
                if ( ! strip_it(*p, pat) ) {
                    ++p;
                    goto end_loop2;
                }
            }
        }

end_loop2:
    end.chunk = c;
    end.cur = p;
    normalize_pos(&end, 0);

    }

    else
        end = GenericEndPos;

    return hlt_bytes_sub(start, end, excpt, ctx);
}

hlt_bytes* hlt_bytes_join(hlt_bytes* sep, hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_type_info* ti = hlt_list_type(l, excpt, ctx);
    hlt_iterator_list i = hlt_list_begin(l, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(l, excpt, ctx);

    int first = 1;
    hlt_bytes* b = hlt_bytes_new(excpt, ctx);

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {

        if ( ! first )
            hlt_bytes_append(b, sep, excpt, ctx);

        // FIXME: The charset handling isn't really right here. We should
        // probably change the to_string methods to take a flag indicating
        // "binary is ok", and then have a hlt_bytes_from_object() function
        // that just passes that back. Or maybe we even need a separate
        // to_bytes() function?
        void* obj = hlt_iterator_list_deref(i, excpt, ctx);
        hlt_string so = hlt_string_from_object(ti, obj, excpt, ctx);
        hlt_bytes* bo = hlt_string_encode(so, Hilti_Charset_UTF8, excpt, ctx);
        hlt_bytes_append(b, bo, excpt, ctx);

        GC_DTOR_GENERIC(obj, ti);
        GC_DTOR(so, hlt_string);
        GC_DTOR(bo, hlt_bytes);

        hlt_iterator_list j = i;
        i = hlt_iterator_list_incr(i, excpt, ctx);
        GC_DTOR(j, hlt_iterator_list);

        first = 0;
    }

    GC_DTOR(i, hlt_iterator_list);
    GC_DTOR(end, hlt_iterator_list);

    return b;
}
