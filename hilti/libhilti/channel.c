/* $Id$
 * 
 * Support functions HILTI's channel data type.
 * 
 */

/* TODO: 
 * - Implement a fine-grained locking strategy. If reading and writing is
 *   performed on different chunks, there is no need to lock the entire channel
 *   for read and write operations.
 * - Substitute the Monitor design (locks and condititions) with a wait-free
 *   implementation based on atomics. This would also obviate the need for a
 *   channel destructor if we can get rid of the pthread code.
 */

#include <string.h>

#include "hilti_intern.h"

#define INITIAL_CHUNK_SIZE (1<<8)
#define MAX_CHUNK_SIZE (1<<14)

static const char* channel_name = "channel";
static const __hlt_string prefix = { 1, "<" };
static const __hlt_string postfix = { 1, ">" };
static const __hlt_string separator = { 1, "," };

// A gc'ed chunk of memory used by the channel.
struct __hlt_channel_chunk
{
    size_t capacity;                /* Maximum number of items. */
    size_t wcnt;                    /* Number of items written. */
    size_t rcnt;                    /* Number of items read. */
    void* data;                     /* Chunk data. */
    __hlt_channel_chunk* next;      /* Pointer to the next chunk. */
};


// Creates a new gc'ed channel chunk.
static __hlt_channel_chunk* __hlt_chunk_create(size_t capacity, int16_t item_size, __hlt_exception* excpt)
{
    __hlt_channel_chunk *chunk = __hlt_gc_malloc_non_atomic(sizeof(__hlt_channel_chunk));
    if ( ! chunk ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    chunk->capacity = capacity;
    chunk->wcnt = chunk->rcnt = 0;

    chunk->data = __hlt_gc_malloc_atomic(capacity * item_size);
    if ( ! chunk->data ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    chunk->next = 0;

    return chunk;
}

// Internal helper function performing a read operation.
static inline void* __hlt_channel_read_item(__hlt_channel* ch)
{
    if ( ch->rc->rcnt == ch->rc->capacity ) {
        assert(ch->rc->next);

        ch->rc = ch->rc->next;
        ch->head = ch->rc->data;
    }

    void* item = ch->head;
    ++ch->rc->rcnt;

    ch->head += ch->type->size;
    --ch->size;

    return item;
}

// Internal helper function performing a write operation.
static inline int __hlt_channel_write_item(__hlt_channel* ch, void* data, __hlt_exception* excpt)
{
    if ( ch->wc->wcnt == ch->wc->capacity ) {
        if ( ch->capacity )
            while ( ch->chunk_cap > ch->capacity - ch->size )
                ch->chunk_cap /= 2;
        else if ( ch->chunk_cap < MAX_CHUNK_SIZE )
            ch->chunk_cap *= 2;

        ch->wc->next = __hlt_chunk_create(ch->chunk_cap, ch->type->size, excpt);
        if ( ! ch->wc->next ) {
            *excpt = __hlt_exception_out_of_memory;
            return 1;
        }

        ch->wc = ch->wc->next;
        ch->tail = ch->wc->data;
    }

    memcpy(ch->tail, data, ch->type->size);
    ++ch->wc->wcnt;

    ch->tail += ch->type->size;
    ++ch->size;

    return 0;
}

const __hlt_string* __hlt_channel_to_string(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_CHANNEL);
    assert(type->num_params == 2);

    const __hlt_string *s = __hlt_string_from_asciiz(channel_name, excpt);
    s = __hlt_string_concat(s, &prefix, excpt);
    if ( *excpt )
        return 0;

    __hlt_type_info** types = (__hlt_type_info**) &type->type_params;
    int i;
    for ( i = 0; i < type->num_params; i++ ) {
        const __hlt_string *t;

        if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj, 0, excpt);
            if ( *excpt )
                return 0;
        }
        else
            t = __hlt_string_from_asciiz(types[i]->tag, excpt);
        
        s = __hlt_string_concat(s, t, excpt);
        if ( *excpt )
            return 0;
        
        if ( i < type->num_params - 1 ) {
            s = __hlt_string_concat(s, &separator, excpt);
            if ( *excpt )
                return 0;
        }
        
    }

    return __hlt_string_concat(s, &postfix, excpt);
}


__hlt_channel* __hlt_channel_new(const __hlt_type_info* type, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_CHANNEL);
    assert(type->num_params == 2);

    __hlt_channel_params* params = (__hlt_channel_params*) &type->type_params;

    __hlt_channel *ch = __hlt_gc_malloc_non_atomic(sizeof(__hlt_channel));
    if ( ! ch ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    ch->type = params->item_type;
    ch->capacity = params->capacity;
    ch->size = 0;

    ch->chunk_cap = INITIAL_CHUNK_SIZE;
    ch->rc = ch->wc = __hlt_chunk_create(ch->chunk_cap, ch->type->size, excpt);
    if ( ! ch->rc ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    ch->head = ch->tail = ch->rc->data;

    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->empty_cv, NULL);
    pthread_cond_init(&ch->full_cv, NULL);

    return ch;
}

void __hlt_channel_destroy(__hlt_channel* ch, __hlt_exception* excpt)
{
    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->empty_cv);
    pthread_cond_destroy(&ch->full_cv);
}

void __hlt_channel_write(__hlt_channel* ch, const __hlt_type_info* type, void* data, __hlt_exception* excpt)
{ 
    pthread_mutex_lock(&ch->mutex);

    if ( __hlt_channel_write_item(ch, data, excpt) )
        goto unlock_exit;

    pthread_cond_signal(&ch->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return;
}

void __hlt_channel_try_write(__hlt_channel* ch, const __hlt_type_info* type, void* data, __hlt_exception* excpt)
{ 
    pthread_mutex_lock(&ch->mutex);

    if ( ch->capacity && ch->size == ch->capacity ) {
        *excpt = __hlt_channel_full;
        goto unlock_exit;
    }

    if ( __hlt_channel_write_item(ch, data, excpt) )
        goto unlock_exit;

    pthread_cond_signal(&ch->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return;
}

void* __hlt_channel_read(__hlt_channel* ch, __hlt_exception* excpt)
{
    pthread_mutex_lock(&ch->mutex);

    while ( ch->size == 0 )
        pthread_cond_wait(&ch->empty_cv, &ch->mutex);

    void *item = __hlt_channel_read_item(ch);
    
    pthread_cond_signal(&ch->full_cv);

    pthread_mutex_unlock(&ch->mutex);
    return item;
}

void* __hlt_channel_try_read(__hlt_channel* ch, __hlt_exception* excpt)
{
    pthread_mutex_lock(&ch->mutex);

    void *item = 0;

    if ( ch->size == 0 ) {
        *excpt = __hlt_channel_empty;
        goto unlock_exit;
    }

    item = __hlt_channel_read_item(ch);
    
    pthread_cond_signal(&ch->full_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return item;
}

__hlt_channel_size __hlt_channel_get_size(__hlt_channel* ch, __hlt_exception* excpt)
{
    return ch->size;
}
