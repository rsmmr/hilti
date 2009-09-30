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

hlt_bytes* hlt_bytes_new(hlt_exception* excpt)
{ 
    return hlt_gc_calloc_non_atomic(1, sizeof(hlt_bytes));
}

hlt_bytes* __hlt_bytes_new_from_data(const int8_t* data, int32_t len, hlt_exception* excpt)
{ 
    hlt_bytes* b = hlt_gc_calloc_non_atomic(1, sizeof(hlt_bytes));
    hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    dst->start = data;
    dst->end = data + len;
    dst->owner = 0;
    add_chunk(b, dst);
    return b;
}

// Returns the number of bytes stored.
hlt_bytes_size hlt_bytes_len(const hlt_bytes* b, hlt_exception* excpt)
{
    hlt_bytes_size len = 0;
    
    for ( const hlt_bytes_chunk* c = b->head; c; c = c->next )
        len += c->end - c->start;
    
    return len;
}

// Returns true if empty.
int8_t hlt_bytes_empty(const hlt_bytes* b, hlt_exception* excpt)
{
    return b->head == 0;
}

// Appends one Bytes object to another.
void hlt_bytes_append(hlt_bytes* b, const hlt_bytes* other, hlt_exception* excpt)
{
    if ( ! other->head )
        // Empty.
        return;
    
    if ( b == other ) {
        *excpt = hlt_exception_value_error;
        return;
    }
    
    // Special case: if the other object has only one chunk, pass it on to
    // the *_raw version which might decide to copy the data over.
    if ( other->head == other->tail )
        return hlt_bytes_append_raw(b, other->head->start, other->head->end - other->head->start, excpt);
    
    for ( const hlt_bytes_chunk* src = other->head; src; src = src->next ) {
        hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        dst->start = src->start;
        dst->end = src->end;
        dst->owner = 0;
        add_chunk(b, dst);
    }
    
}

void hlt_bytes_append_raw(hlt_bytes* b, const int8_t* raw, hlt_bytes_size len, hlt_exception* excpt)
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

    hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    
    // If the raw data is small, we copy it over into a newly allocated block
    // to avoid fragmentation.
    if ( len <= MAX_COPY_SIZE ) {
        int8_t* mem = hlt_gc_malloc_atomic(ALLOC_SIZE);
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

static hlt_bytes_pos PosEnd = { 0, 0 };

static inline int8_t is_end(hlt_bytes_pos pos)
{
    return pos.chunk == 0;
}

hlt_bytes* hlt_bytes_sub(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt)
{
    if ( hlt_bytes_pos_eq(start, end, excpt) )
        // Return an empty bytes object.
        return hlt_bytes_new(excpt);
    
    if ( is_end(start) ) {
        *excpt = hlt_exception_value_error;
        return 0;
    }
    
    hlt_bytes* b = hlt_bytes_new(excpt);

    // Special case: if both positions are inside the same chunk, it's easy.
    if ( start.chunk == end.chunk || (is_end(end) && start.chunk->next == 0) ) {
        hlt_bytes_chunk* c = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        c->start = start.cur;
        c->end = end.cur ? end.cur : start.chunk->end;
        c->owner = 0;
        add_chunk(b, c);
        return b;
    }
    
    // Create the first chunk of the sub-bytes. 
    hlt_bytes_chunk* first = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
    first->start = start.cur;
    first->end = start.chunk->end;
    first->owner = 0;
    add_chunk(b, first);
    
    // Copy the chunks in between.
    for ( const hlt_bytes_chunk* c = start.chunk->next; is_end(end) || c != end.chunk; c = c->next ) {
        
        if ( ! c ) {
            if ( is_end(end) )
                break;
            
            // Start and end are not part of the same Bytes object!
            *excpt = hlt_exception_value_error;
            return 0;
        }
        
        hlt_bytes_chunk* dst = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        dst->start = c->start;
        dst->end = c->end;
        dst->owner = 0;
        add_chunk(b, dst);
    }

    if ( ! is_end(end) ) {
        // Create the last chunk of the sub-bytes.
        hlt_bytes_chunk* last = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes_chunk));
        last->start = end.chunk->start;
        last->end = end.cur;
        last->owner = 0;
        add_chunk(b, last);
    }
    
    return b;
}

static const int8_t* hlt_bytes_sub_raw_internal(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt)
{    
    hlt_bytes_size len = hlt_bytes_pos_diff(start, end, excpt);

    if ( len == 0 ) {
        // Can't happen because we should have be in the easy case. 
        *excpt = hlt_exception_internal_error;
        return 0;
        }
    
    if ( len < 0 ) {
        // Wrong order of arguments.
        *excpt = hlt_exception_value_error;
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
            *excpt = hlt_exception_value_error;
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


const int8_t* hlt_bytes_sub_raw(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt)
{
    // The easy case: start and end are within the same chunk.
    if ( start.chunk == end.chunk )
        return start.cur;
    
    // We split the rest into its own function to make it easier for the
    // compiler to inline the fast case.
    return hlt_bytes_sub_raw_internal(start, end, excpt);
}

const int8_t* hlt_bytes_to_raw(const hlt_bytes* b, hlt_exception* excpt)
{
    hlt_bytes_pos begin = hlt_bytes_begin(b, excpt);
    hlt_bytes_pos end = hlt_bytes_end(excpt);
    return hlt_bytes_sub_raw(begin, end, excpt);
}

int8_t __hlt_bytes_extract_one(hlt_bytes_pos* pos, const hlt_bytes_pos end, hlt_exception* excpt)
{
    if ( is_end(*pos) || hlt_bytes_pos_eq(*pos, end, excpt) ) {
        *excpt = hlt_exception_value_error;
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

hlt_bytes_pos hlt_bytes_offset(const hlt_bytes* b, hlt_bytes_size pos, hlt_exception* excpt)
{
    if ( pos < 0 )
        pos += hlt_bytes_len(b, excpt);
    
    hlt_bytes_chunk* c;
    for ( c = b->head; pos >= (c->end - c->start); c = c->next ) {
        
        if ( ! c ) {
            // Position is out range.
            *excpt = hlt_exception_value_error;
            return PosEnd;
        }
        
        pos -= (c->end - c->start);
        
    }
        
    hlt_bytes_pos p;
    p.chunk = c;
    p.cur = c->start + pos;
    return p;
}

hlt_bytes_pos hlt_bytes_begin(const hlt_bytes* b, hlt_exception* excpt)
{
    if ( ! b->head )
        return PosEnd;
    
    hlt_bytes_pos p;
    p.chunk = b->head;
    p.cur = b->head ? b->head->start : 0;
    return p;
}

hlt_bytes_pos hlt_bytes_end(hlt_exception* excpt)
{
    return PosEnd;
}

int8_t hlt_bytes_pos_deref(hlt_bytes_pos pos, hlt_exception* excpt)
{
    if ( is_end(pos) ) {
        // Position is out range.
        *excpt = hlt_exception_value_error;
        return 0;
    }
    
    return *pos.cur;
}

hlt_bytes_pos hlt_bytes_pos_incr(hlt_bytes_pos old, hlt_exception* excpt)
{
    if ( is_end(old) )
        // Fail silently. 
        return old;

    hlt_bytes_pos pos = old;
    
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

hlt_bytes_pos hlt_bytes_pos_incr_by(hlt_bytes_pos old, int32_t n, hlt_exception* excpt)
{
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
void hlt_bytes_pos_decr(hlt_bytes_pos* pos, hlt_exception* excpt)
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

int8_t hlt_bytes_pos_eq(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception* excpt)
{
    return pos1.cur == pos2.cur && pos1.chunk == pos2.chunk;
}

// Returns the number of bytes from pos1 to pos2 (not counting pos2).
hlt_bytes_size hlt_bytes_pos_diff(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception* excpt)
{
    if ( hlt_bytes_pos_eq(pos1, pos2, excpt) )
        return 0;
    
    if ( is_end(pos1) && ! is_end(pos2) ) {
        // Invalid starting position.
        *excpt = hlt_exception_value_error;
        return 0;
    }
    
    // Special case: both inside same chunk.
    if ( pos1.chunk == pos2.chunk ) {
        if ( pos1.cur > pos2.cur ) {
            // Invalid starting position.
            *excpt = hlt_exception_value_error;
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
            *excpt = hlt_exception_value_error;
            return 0;
        }
        
        n += c->end - c->start;
    }
        
    if ( c )
        // Count last chunk.
        n += pos2.cur - pos2.chunk->start;
    
    return n;
}

hlt_string hlt_bytes_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt)
{
    hlt_string dst = 0; 
    
    char buf[] = "                ";
    
    const hlt_bytes* b = *((const hlt_bytes**)obj);

    // FIXME: I'm sure we can do this more efficiently ...
    for ( const hlt_bytes_chunk* c = b->head; c; c = c->next ) {
        for ( const int8_t* p = c->start; p < c->end; ++p ) {
            unsigned char c = (unsigned char) *p;
        
            if ( isprint(c) ) 
                dst = hlt_string_concat(dst, hlt_string_from_data((int8_t*)&c, 1, excpt), excpt);
            else {
                snprintf(buf, sizeof(buf) - 1, "\\x%02x", c);
                dst = hlt_string_concat(dst, hlt_string_from_asciiz(buf, excpt), excpt);
            }
        }
    }
    
    return dst;
}

void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt)
{
    if ( ! cookie ) {
        
        if ( hlt_bytes_pos_eq(start, end, excpt) ) {
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
