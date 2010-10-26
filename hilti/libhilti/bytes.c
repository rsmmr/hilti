// $Id$

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "hilti.h"

struct hlt_bytes_chunk {
    const int8_t* start;          // Pointer to first data byte.
    const int8_t* end;            // Pointer one beyond the last data byte.
    struct hlt_bytes_chunk* next; // Successor in bytes object.
    struct hlt_bytes_chunk* prev; // Predecessor in bytes object.
    
    int8_t owner;           // True if we "own" the data, i.e., allocated it.
    int8_t frozen;          // True if the object has been frozen.
    int16_t free;           // Bytes still available in allocated block. Only valid if owner. 
};

struct hlt_bytes {
    hlt_bytes_chunk* head;  // First chunk.
    hlt_bytes_chunk* tail;  // Last chunk.
};

// If a chunk of data is of size is less than this, we can decide to copy it
// into space we allocated ourselves.
static const int16_t MAX_COPY_SIZE = 256;

// If we allocate a chunk of a data ourselves, it will be of this size.
static const int16_t ALLOC_SIZE = 1024;

static void add_chunk(hlt_bytes* b, hlt_bytes_chunk* c)
{
    assert(b);
    assert(c);
    
    if ( b->tail ) {
        assert(b->head);
        c->prev = b->tail;
        c->next = 0;
        b->tail->next = c;
        b->tail = c;
    }
    else {
        assert( ! b->head );
        c->prev = c->next = 0;
        b->head = b->tail = c;
    }
}

hlt_bytes* hlt_bytes_new(hlt_exception** excpt, hlt_execution_context* ctx)
{ 
    return hlt_gc_calloc_non_atomic(1, sizeof(hlt_bytes));
}

hlt_bytes* hlt_bytes_new_from_data(const int8_t* data, int32_t len, hlt_exception** excpt, hlt_execution_context* ctx)
{ 
    hlt_bytes* b = hlt_gc_calloc_non_atomic(1, sizeof(hlt_bytes));
    hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    dst->start = data;
    dst->end = data + len;
    dst->owner = 0;
    dst->frozen = 0;

    add_chunk(b, dst);
    
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
    
    for ( const hlt_bytes_chunk* c = b->head; c; c = c->next )
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
    
    return b->head == 0;
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
    
    if ( ! other->head )
        // Empty.
        return;
    
    // Special case: if the other object has only one chunk, pass it on to
    // the *_raw version which might decide to copy the data over.
    if ( other->head == other->tail )
        return hlt_bytes_append_raw(b, other->head->start, other->head->end - other->head->start, excpt, ctx);
    
    for ( const hlt_bytes_chunk* src = other->head; src; src = src->next ) {
        hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        dst->start = src->start;
        dst->end = src->end;
        dst->owner = 0;
        dst->frozen = 0;
        add_chunk(b, dst);
    }
}

void hlt_bytes_append_raw(hlt_bytes* b, const int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }
    
    if ( hlt_bytes_is_frozen(b, excpt, ctx) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }
    
    if ( ! len )
        // Empty.
        return;
    
    // Special case: see if we can copy it into the last chunk.
    if ( b->tail && b->tail->owner && b->tail->free >= len ) {
        memcpy((int8_t*)b->tail->end, raw, len); // const cast is safe because we allocated the block.
        b->tail->end += len;
        b->tail->free -= len;
        return;
    }

    hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    
    // If the raw data is small, we copy it over into a newly allocated block
    // to avoid fragmentation.
    if ( len <= MAX_COPY_SIZE ) {
        int8_t* mem = hlt_gc_malloc_atomic(ALLOC_SIZE);
        dst->start = mem;
        dst->end = dst->start + len;
        dst->owner = 1;
        dst->frozen = 0;
        dst->free = ALLOC_SIZE - (dst->end - dst->start);
        memcpy(mem, raw, len);
    }
    
    else {
        // Otherwise, we link to the data directly.
        dst->start = raw;
        dst->end = raw + len;
        dst->owner = 0;
        dst->frozen = 0;
    }
    
    add_chunk(b, dst);
}

static hlt_bytes_pos GenericEndPos = { 0, 0 };

static inline int8_t is_end(hlt_bytes_pos pos)
{
    return pos.chunk == 0 || (!pos.cur) || pos.cur >= pos.chunk->end;
}

static inline void normalize_pos(hlt_bytes_pos* pos)
{
    if ( ! pos->chunk) 
        return;
    
    // If the pos was previously an end position, but now new data has been
    // added, adjust it so that it's pointing to the next byte.
    if ( (! pos->cur || pos->cur >= pos->chunk->end) && pos->chunk->next ) {
        pos->chunk = pos->chunk->next;
        pos->cur = pos->chunk->start;
    }
}

hlt_bytes* hlt_bytes_copy(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_pos begin = hlt_bytes_begin(b, excpt, ctx);
    hlt_bytes_pos end = hlt_bytes_end(b, excpt, ctx);
    return hlt_bytes_sub(begin, end, excpt, ctx);
}

hlt_bytes* hlt_bytes_sub(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start);
    normalize_pos(&end);
    
    if ( hlt_bytes_pos_eq(start, end, excpt, ctx) )
        // Return an empty bytes object.
        return hlt_bytes_new(excpt, ctx);
    
    if ( is_end(start) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }
    
    hlt_bytes* b = hlt_bytes_new(excpt, ctx);

    // Special case: if both positions are inside the same chunk, it's easy.
    if ( start.chunk == end.chunk || (is_end(end) && start.chunk->next == 0) ) {
        hlt_bytes_chunk* c = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        c->start = start.cur;
        c->end = end.cur ? end.cur : start.chunk->end;
        c->owner = 0;
        c->frozen = 0;
        add_chunk(b, c);
        return b;
    }
    
    // Create the first chunk of the sub-bytes. 
    hlt_bytes_chunk* first = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    first->start = start.cur;
    first->end = start.chunk->end;
    first->owner = 0;
    first->frozen = 0;
    add_chunk(b, first);
    
    // Copy the chunks in between.
    for ( const hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {
        
        if ( ! c ) {
            if ( is_end(end) )
                break;
            
            // Start and end are not part of the same Bytes object!
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }
        
        hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        dst->start = c->start;
        dst->end = c->end;
        dst->owner = 0;
        dst->frozen = 0;
        add_chunk(b, dst);
    }

    if ( ! is_end(end) ) {
        // Create the last chunk of the sub-bytes.
        hlt_bytes_chunk* last = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        last->start = end.chunk->start;
        last->end = end.cur;
        last->owner = 0;
        last->frozen = 0;
        add_chunk(b, last);
    }
    
    return b;
}

static const int8_t* hlt_bytes_sub_raw_internal(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{    
    normalize_pos(&start);
    normalize_pos(&end);
    
    hlt_bytes_size len = hlt_bytes_pos_diff(start, end, excpt, ctx);

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
    int8_t* mem = hlt_gc_malloc_atomic(len);
    int8_t* p = mem;
    
    // Copy the first chunk.
    hlt_bytes_size n = start.chunk->end - start.cur;
    memcpy(p, start.cur, n);
    p += n;
    
    // Copy the chunks in between.
    for ( const hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {
        
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


const int8_t* hlt_bytes_sub_raw(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start);
    normalize_pos(&end);
    
    // The easy case: start and end are within the same chunk.
    if ( start.chunk == end.chunk )
        return start.cur;
    
    // We split the rest into its own function to make it easier for the
    // compiler to inline the fast case.
    return hlt_bytes_sub_raw_internal(start, end, excpt, ctx);
}

const int8_t* hlt_bytes_to_raw(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }
    
    hlt_bytes_pos begin = hlt_bytes_begin(b, excpt, ctx);
    hlt_bytes_pos end = hlt_bytes_end(b, excpt, ctx);
    return hlt_bytes_sub_raw(begin, end, excpt, ctx);
}

int8_t __hlt_bytes_extract_one(hlt_bytes_pos* pos, hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(pos);
    normalize_pos(&end);
    
    if ( is_end(*pos) || hlt_bytes_pos_eq(*pos, end, excpt, ctx) ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        return 0;
    }

    // Extract byte.
    int8_t b = *(pos->cur);

    // Increase iterator. 
    if ( pos->cur < pos->chunk->end - 1 ) 
        // We stay inside chunk.
        ++pos->cur;
    
    else {
        if ( pos->chunk->next ) {
            // Switch to next chunk.
            pos->chunk = pos->chunk->next;
            pos->cur = pos->chunk->start;
        }
        else {  
            // End reached.
            pos->cur = pos->chunk->end;
        }
    }
    
    return b;
}

hlt_bytes_pos hlt_bytes_offset(const hlt_bytes* b, hlt_bytes_size pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        hlt_bytes_pos p;
        return p;
    }
    
    if ( pos < 0 )
        pos += hlt_bytes_len(b, excpt, ctx);
    
    hlt_bytes_chunk* c;
    for ( c = b->head; pos >= (c->end - c->start); c = c->next ) {
        
        if ( ! c ) {
            // Position is out range.
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return GenericEndPos;
        }
        
        pos -= (c->end - c->start);
        
    }
        
    hlt_bytes_pos p;
    p.chunk = c;
    p.cur = c->start + pos;
    return p;
}

hlt_bytes_pos hlt_bytes_begin(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        hlt_bytes_pos p;
        return p;
    }

    if ( hlt_bytes_len(b, excpt, ctx) == 0 )
        return hlt_bytes_end(b, excpt, ctx);

    hlt_bytes_pos p;
    p.chunk = b->head;
    p.cur = b->head ? b->head->start : 0;
    return p;
}

hlt_bytes_pos hlt_bytes_end(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        hlt_bytes_pos p;
        return p;
    }
    
    hlt_bytes_pos p;
    p.chunk = b->tail;
    p.cur = b->tail ? b->tail->end : 0;
    return p;
}

hlt_bytes_pos hlt_bytes_generic_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GenericEndPos;
}

void hlt_bytes_freeze(const hlt_bytes* b, int8_t freeze, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }
    
    if ( (! b->tail) && freeze ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }
    
    b->tail->frozen = freeze;
}

int8_t hlt_bytes_is_frozen(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx) 
{
    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }
    
    return b->tail ? b->tail->frozen : 0;
}

int8_t hlt_bytes_pos_is_frozen(hlt_bytes_pos pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos);
    
    if ( ! pos.chunk )
        return 0;
    
    // Go to last chunk of string.
    const hlt_bytes_chunk* c;
    for ( c = pos.chunk; c->next; c = c->next );
    
    return c->frozen;
}

int8_t hlt_bytes_pos_deref(hlt_bytes_pos pos, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos);
    
    if ( is_end(pos) ) {
        // Position is out range.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }
    
    return *pos.cur;
}

hlt_bytes_pos hlt_bytes_pos_incr(hlt_bytes_pos old, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&old);
    
    if ( is_end(old) )
        // Fail silently. 
        return old;

    hlt_bytes_pos pos = old;
    
    // Can we stay inside the same chunk?
    if ( pos.cur < pos.chunk->end - 1 ) {
        ++pos.cur;
        return pos;
    }

    if ( pos.chunk->next ) {
        // Need to switch chunk.
        pos.chunk = pos.chunk->next;
        pos.cur = pos.chunk->start;
    }
    else
        // End reached.
        pos.cur = pos.chunk->end;
    
    return pos;
}

hlt_bytes_pos hlt_bytes_pos_incr_by(hlt_bytes_pos old, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! n ) 
        return old;
    
    normalize_pos(&old);
    
    if ( is_end(old) )
        // Fail silently. 
        return old;

    hlt_bytes_pos pos = old;

    while ( 1 ) {
        // Can we stay inside the same chunk?
        if ( pos.cur + n < pos.chunk->end ) {
            pos.cur += n;
            return pos;
        }
        
        n -= pos.chunk->end - pos.cur;
        
        if ( pos.chunk->next ) {
            // Need to switch chunk.
            pos.chunk = pos.chunk->next;
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
void hlt_bytes_pos_decr(hlt_bytes_pos* pos, hlt_exception** excpt, hlt_execution_context* ctx)
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

int8_t hlt_bytes_pos_eq(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos1);
    normalize_pos(&pos2);

    if ( is_end(pos1) && is_end(pos2) )
        return 1;
    
    return pos1.cur == pos2.cur && pos1.chunk == pos2.chunk;
}

// Returns the number of bytes from pos1 to pos2 (not counting pos2).
hlt_bytes_size hlt_bytes_pos_diff(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&pos1);
    normalize_pos(&pos2);
    
    if ( hlt_bytes_pos_eq(pos1, pos2, excpt, ctx) )
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
    hlt_bytes_size n = pos1.chunk->end - pos1.cur;

    // Count intermediary chunks.
    const hlt_bytes_chunk* c;
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
    
    char buf[] = "                ";
    
    const hlt_bytes* b = *((const hlt_bytes**)obj);

    if ( ! b ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }
    
    // FIXME: I'm sure we can do this more efficiently ...
    for ( const hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        for ( const int8_t* p = c->start; p < c->end; ++p ) {
            unsigned char c = (unsigned char) *p;
        
            if ( isprint(c) && c < 128 ) 
                dst = hlt_string_concat(dst, hlt_string_from_data((int8_t*)&c, 1, excpt, ctx), excpt, ctx);
            else {
                snprintf(buf, sizeof(buf) - 1, "\\x%02x", c);
                dst = hlt_string_concat(dst, hlt_string_from_asciiz(buf, excpt, ctx), excpt, ctx);
            }
        }
    }
    
    return dst;
}

void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    normalize_pos(&start);
    normalize_pos(&end);
    
    if ( ! cookie ) {
        
        if ( hlt_bytes_pos_eq(start, end, excpt, ctx) ) {
            block->start = block->end = start.cur;
            block->next = 0;
            return 0;
        }
        
        if ( start.chunk != end.chunk  ) {
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
    hlt_bytes_pos p1 = hlt_bytes_begin(b1, excpt, ctx);
    hlt_bytes_pos p2 = hlt_bytes_begin(b2, excpt, ctx);

    while ( true ) {
        
        if ( is_end(p1) )
            return is_end(p2) ? 0 : 1;
        
        if ( is_end(p2) )
            return -1;
        
        int8_t c1 = hlt_bytes_pos_deref(p1, excpt, ctx);
        int8_t c2 = hlt_bytes_pos_deref(p2, excpt, ctx);
        
        if ( c1 != c2 )
            return c1 > c2 ? -1 : 1;
        
        p1 = hlt_bytes_pos_incr(p1, excpt, ctx);
        p2 = hlt_bytes_pos_incr(p2, excpt, ctx);
    }
}
