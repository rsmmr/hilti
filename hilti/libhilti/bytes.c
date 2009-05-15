/* $Id$
 * 
 * Support functions for HILTI's bytes data type.
 * 
 * A Bytes object is internally represented as a list of variable-size data
 * chunks. While the list can be modified (e.g., adding another chunk of data
 * will append an entry to the list), the data chunks are immutable and can
 * therefore be shared across multiple Bytes objects. For example, creating a
 * new Bytes object containing only a subsequence of an existing one needs to
 * create only a new list pointing to the appropiate chunks 
 * 
 * There is one optimization of this basic scheme: To avoid fragmentation
 * when adding many tiny data chunks, in certain cases new data is copied
 * into an existing chunk. This will however never change any content a chunk
 * already has (because other Bytes objects might use that as well) but only
 * *append* to a chunk's data if the initial allocation was large enough. To
 * avoid conflicts, only the Bytes object which allocated a chunk in the
 * first place is allowed to extend it in this way; that object is called the
 * chunk's "owner".
 * 
 * To allow for efficient indexing and iteration over a Bytes objects, there
 * are also Position objects. Each Position is associated with a particular
 * bytes objects and can be used to locate a specific byte. Creating a
 * Position from an offset into the Bytes can be potentially expensive if the
 * target position is far into the chunk list; however once created,
 * Positions can be dererenced, incremented, and decremented efficiently. 
 * 
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "hilti_intern.h"

struct __hlt_bytes_chunk {
    const int8_t* start;            // Pointer to first data byte.
    const int8_t* end;              // Pointer one beyond the last data byte.
    struct __hlt_bytes_chunk* next; // Successor in bytes object.
    struct __hlt_bytes_chunk* prev; // Predecessor in bytes object.
    
    int8_t owner;           // True if we "own" the data, i.e., allocated it.
    int16_t free;           // Bytes still available in allocated block. Only valid if owner. 
};

struct __hlt_bytes {
    __hlt_bytes_chunk* head;  // First chunk.
    __hlt_bytes_chunk* tail;  // Last chunk.
};

// If a chunk of data is of size is less than this, we can decide to copy it
// into space we allocated ourselves.
static const int16_t MAX_COPY_SIZE = 256;

// If we allocate a chunk of a data ourselves, it will be of this size.
static const int16_t ALLOC_SIZE = 1024;

static void add_chunk(__hlt_bytes* b, __hlt_bytes_chunk* c)
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

__hlt_bytes* __hlt_bytes_new(__hlt_exception* excpt)
{ 
    return __hlt_gc_calloc_non_atomic(1, sizeof(__hlt_bytes));
}

__hlt_bytes* __hlt_bytes_new_from_data(const int8_t* data, int32_t len, __hlt_exception* excpt)
{ 
    __hlt_bytes* b = __hlt_gc_calloc_non_atomic(1, sizeof(__hlt_bytes));
    __hlt_bytes_chunk* dst = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
    dst->start = data;
    dst->end = data + len;
    dst->owner = 0;
    add_chunk(b, dst);
    return b;
}

// Returns the number of bytes stored.
__hlt_bytes_size __hlt_bytes_len(const __hlt_bytes* b, __hlt_exception* excpt)
{
    __hlt_bytes_size len = 0;
    
    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next )
        len += c->end - c->start;
    
    return len;
}

// Returns true if empty.
int8_t __hlt_bytes_empty(const __hlt_bytes* b, __hlt_exception* excpt)
{
    return b->head == 0;
}

// Appends one Bytes object to another.
void __hlt_bytes_append(__hlt_bytes* b, const __hlt_bytes* other, __hlt_exception* excpt)
{
    if ( ! other->head )
        // Empty.
        return;
    
    if ( b == other ) {
        *excpt = __hlt_exception_value_error;
        return;
    }
    
    // Special case: if the other object has only one chunk, pass it on to
    // the *_raw version which might decide to copy the data over.
    if ( other->head == other->tail )
        return __hlt_bytes_append_raw(b, other->head->start, other->head->end - other->head->start, excpt);
    
    for ( const __hlt_bytes_chunk* src = other->head; src; src = src->next ) {
        __hlt_bytes_chunk* dst = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
        dst->start = src->start;
        dst->end = src->end;
        dst->owner = 0;
        add_chunk(b, dst);
    }
    
}

void __hlt_bytes_append_raw(__hlt_bytes* b, const int8_t* raw, __hlt_bytes_size len, __hlt_exception* excpt)
{
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

    __hlt_bytes_chunk* dst = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
    
    // If the raw data is small, we copy it over into a newly allocated block
    // to avoid fragmentation.
    if ( len <= MAX_COPY_SIZE ) {
        int8_t* mem = __hlt_gc_malloc_atomic(ALLOC_SIZE);
        dst->start = mem;
        dst->end = dst->start + len;
        dst->owner = 1;
        dst->free = ALLOC_SIZE - (dst->end - dst->start);
        memcpy(mem, raw, len);
    }
    
    else {
        // Otherwise, we link to the data directly.
        dst->start = raw;
        dst->end = raw + len;
        dst->owner = 0;
    }
    
    add_chunk(b, dst);
}

static __hlt_bytes_pos PosEnd = { 0, 0 };

static inline int8_t is_end(__hlt_bytes_pos pos)
{
    return pos.chunk == 0;
}

__hlt_bytes* __hlt_bytes_sub(__hlt_bytes_pos start, __hlt_bytes_pos end, __hlt_exception* excpt)
{
    if ( __hlt_bytes_pos_eq(start, end, excpt) )
        // Return an empty bytes object.
        return __hlt_bytes_new(excpt);
    
    if ( is_end(start) ) {
        *excpt = __hlt_exception_value_error;
        return 0;
    }
    
    __hlt_bytes* b = __hlt_bytes_new(excpt);

    // Special case: if both positions are inside the same chunk, it's easy.
    if ( start.chunk == end.chunk || (is_end(end) && start.chunk->next == 0) ) {
        __hlt_bytes_chunk* c = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
        c->start = start.cur;
        c->end = end.cur ? end.cur : start.chunk->end;
        c->owner = 0;
        add_chunk(b, c);
        return b;
    }
    
    // Create the first chunk of the sub-bytes. 
    __hlt_bytes_chunk* first = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
    first->start = start.cur;
    first->end = start.chunk->end;
    first->owner = 0;
    add_chunk(b, first);
    
    // Copy the chunks in between.
    for ( const __hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {
        
        if ( ! c ) {
            if ( is_end(end) )
                break;
            
            // Start and end are not part of the same Bytes object!
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        __hlt_bytes_chunk* dst = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
        dst->start = c->start;
        dst->end = c->end;
        dst->owner = 0;
        add_chunk(b, dst);
    }

    if ( ! is_end(end) ) {
        // Create the last chunk of the sub-bytes.
        __hlt_bytes_chunk* last = __hlt_gc_malloc_non_atomic(sizeof(__hlt_bytes_chunk));
        last->start = end.chunk->start;
        last->end = end.cur;
        last->owner = 0;
        add_chunk(b, last);
    }
    
    return b;
}

static const int8_t* __hlt_bytes_sub_raw_internal(__hlt_bytes_pos start, __hlt_bytes_pos end, __hlt_exception* excpt)
{    
    __hlt_bytes_size len = __hlt_bytes_pos_diff(start, end, excpt);

    if ( len == 0 ) {
        // Can't happen because we should have be in the easy case. 
        *excpt = __hlt_exception_internal_error;
        return 0;
        }
    
    if ( len < 0 ) {
        // Wrong order of arguments.
        *excpt = __hlt_exception_value_error;
        return 0;
    }

    // The potentially expensive case: we need to copy the data into a
    // continous memory region. 
    int8_t* mem = __hlt_gc_malloc_atomic(len);
    int8_t* p = mem;
    
    // Copy the first chunk.
    __hlt_bytes_size n = start.chunk->end - start.cur;
    memcpy(p, start.cur, n);
    p += n;
    
    // Copy the chunks in between.
    for ( const __hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {
        
        if ( ! c ) {
            
            if ( is_end(end) )
                break;
            
            // Start and end are not part of the same Bytes object!
            *excpt = __hlt_exception_value_error;
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


const int8_t* __hlt_bytes_sub_raw(__hlt_bytes_pos start, __hlt_bytes_pos end, __hlt_exception* excpt)
{
    // The easy case: start and end are within the same chunk.
    if ( start.chunk == end.chunk )
        return start.cur;
    
    // We split the rest into its own function to make it easier for the
    // compiler to inline the fast case.
    return __hlt_bytes_sub_raw_internal(start, end, excpt);
}

int8_t __hlt_bytes_extract_one(__hlt_bytes_pos* pos, const __hlt_bytes_pos end, __hlt_exception* excpt)
{
    if ( is_end(*pos) || __hlt_bytes_pos_eq(*pos, end, excpt) ) {
        *excpt = __hlt_exception_value_error;
        return 0;
    }

    // Extract byte.
    int8_t b = *(pos->cur);

    // Increate iterator. 
    
    if ( pos->cur < pos->chunk->end - 1 ) 
        // We stay inside chunk.
        ++pos->cur;
        
    else {
        // Switch to next chunk.
        pos->chunk = pos->chunk->next;
        if ( pos->chunk )
            pos->cur = pos->chunk->start;
        else   
            // End reached.
            *pos = PosEnd;
    }
    
    return b;
}

__hlt_bytes_pos __hlt_bytes_offset(const __hlt_bytes* b, __hlt_bytes_size pos, __hlt_exception* excpt)
{
    if ( pos < 0 )
        pos += __hlt_bytes_len(b, excpt);
    
    __hlt_bytes_chunk* c;
    for ( c = b->head; pos >= (c->end - c->start); c = c->next ) {
        
        if ( ! c ) {
            // Position is out range.
            *excpt = __hlt_exception_value_error;
            return PosEnd;
        }
        
        pos -= (c->end - c->start);
        
    }
        
    __hlt_bytes_pos p;
    p.chunk = c;
    p.cur = c->start + pos;
    return p;
}

__hlt_bytes_pos __hlt_bytes_begin(const __hlt_bytes* b, __hlt_exception* excpt)
{
    if ( ! b->head )
        return PosEnd;
    
    __hlt_bytes_pos p;
    p.chunk = b->head;
    p.cur = b->head ? b->head->start : 0;
    return p;
}

__hlt_bytes_pos __hlt_bytes_end(const __hlt_bytes* b, __hlt_exception* excpt)
{
    return PosEnd;
}

int8_t __hlt_bytes_pos_deref(__hlt_bytes_pos pos, __hlt_exception* excpt)
{
    if ( is_end(pos) ) {
        // Position is out range.
        *excpt = __hlt_exception_value_error;
        return 0;
    }
    
    return *pos.cur;
}

__hlt_bytes_pos __hlt_bytes_pos_incr(__hlt_bytes_pos old, __hlt_exception* excpt)
{
    if ( is_end(old) )
        // Fail silently. 
        return old;

    __hlt_bytes_pos pos = old;
    
    // Can we stay inside the same chunk?
    if ( pos.cur < pos.chunk->end - 1 ) {
        ++pos.cur;
        return pos;
    }

    // Need to switch chunk.
    pos.chunk = pos.chunk->next;
    if ( ! pos.chunk ) {
        // End reached.
        return PosEnd;
    }
    
    pos.cur = pos.chunk->start;
    return pos;
}

__hlt_bytes_pos __hlt_bytes_pos_incr_by(__hlt_bytes_pos old, int32_t n, __hlt_exception* excpt)
{
    if ( is_end(old) )
        // Fail silently. 
        return old;

    __hlt_bytes_pos pos = old;

    while ( 1 ) {
        // Can we stay inside the same chunk?
        if ( pos.cur + n < pos.chunk->end ) {
            pos.cur += n;
            return pos;
        }
        
        // Need to switch chunk.
        n -= pos.chunk->end - pos.cur;
        pos.chunk = pos.chunk->next;
        pos.cur = pos.chunk->start;
        
        if ( ! pos.chunk ) {
            // End reached.
            return PosEnd;
        }
    }
    
    return pos;
}


#if 0
// This is a bit tricky because pos_end() doesn't return anything suitable
// for iteration.  Let's leave this out for now until we know whether we
// actually need reverse iteration. 
void __hlt_bytes_pos_decr(__hlt_bytes_pos* pos, __hlt_exception* excpt)
{
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
        *pos = PosEnd;
        return;
    }
    
    pos->cur = pos->chunk->end - 1;
    pos->end = pos->chunk->end;
    return;
}
#endif

int8_t __hlt_bytes_pos_eq(__hlt_bytes_pos pos1, __hlt_bytes_pos pos2, __hlt_exception* excpt)
{
    return pos1.cur == pos2.cur && pos1.chunk == pos2.chunk;
}

// Returns the number of bytes from pos1 to pos2 (not counting pos2).
__hlt_bytes_size __hlt_bytes_pos_diff(__hlt_bytes_pos pos1, __hlt_bytes_pos pos2, __hlt_exception* excpt)
{
    if ( __hlt_bytes_pos_eq(pos1, pos2, excpt) )
        return 0;
    
    if ( is_end(pos1) ) {
        // Invalid starting position.
        *excpt = __hlt_exception_value_error;
        return 0;
    }
    
    // Special case: both inside same chunk.
    if ( pos1.chunk == pos2.chunk ) {
        if ( pos1.cur > pos2.cur ) {
            // Invalid starting position.
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        return pos2.cur - pos1.cur;
    }

    // Count first chunk.
    __hlt_bytes_size n = pos1.chunk->end - pos1.cur;

    // Count intermediary chunks.
    const __hlt_bytes_chunk* c;
    for ( c = pos1.chunk->next; is_end(pos2) || c != pos2.chunk; c = c->next ) {
        if ( ! c ) {
            if ( is_end(pos2) )
                break;
            
            // The two positions are not part of the same Bytes object!
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        n += c->end - c->start;
    }
        
    if ( c )
        // Count last chunk.
        n += pos2.cur - pos2.chunk->start;
    
    return n;
}

const __hlt_string* __hlt_bytes_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    const __hlt_string* dst = 0; 
    
    char buf[] = "                ";
    
    const __hlt_bytes* b = *((const __hlt_bytes**)obj);

    // FIXME: I'm sure we can do this more efficiently ...
    for ( const __hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        for ( const int8_t* p = c->start; p < c->end; ++p ) {
            unsigned char c = (unsigned char) *p;
        
            if ( isprint(c) ) 
                dst = __hlt_string_concat(dst, __hlt_string_from_data((int8_t*)&c, 1, excpt), excpt);
            else {
                snprintf(buf, sizeof(buf) - 1, "\\x%02x", c);
                dst = __hlt_string_concat(dst, __hlt_string_from_asciiz(buf, excpt), excpt);
            }
        }
    }
    
    return dst;
}
