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
#include <stdio.h>
#include <pthread.h>

#include "channel.h"
#include "string_.h"

#define INITIAL_CHUNK_SIZE (1<<8)
#define MAX_CHUNK_SIZE (1<<14)

// A gc'ed chunk of memory used by the channel.
typedef struct __hlt_channel_chunk {
    size_t capacity;                 /* Maximum number of items. */
    size_t wcnt;                     /* Number of items written. */
    size_t rcnt;                     /* Number of items read. */
    void* data;                      /* Chunk data. */
    struct __hlt_channel_chunk* next; /* Pointer to the next chunk. */
} hlt_channel_chunk;

struct __hlt_channel {
    __hlt_gchdr __gchdr;    // Header for memory management.
    const hlt_type_info* type;      /* Type information of the channel's data type. */
    hlt_channel_capacity capacity;      /* Maximum number of channel items. */
    volatile hlt_channel_capacity size; /* Current number of channel items. */

    size_t chunk_cap;               /* Chunk capacity of the next chunk. */
    hlt_channel_chunk* rc;          /* Pointer to the reader chunk. */
    hlt_channel_chunk* wc;          /* Pointer to the writer chunk. */
    void* head;                     /* Pointer to the next item to read. */
    void* tail;                     /* Pointer to the first empty slot for writing. */

    pthread_mutex_t mutex;          /* Synchronizes access to the channel. */
    pthread_cond_t empty_cv;        /* Condition variable for an empty channel. */
    pthread_cond_t full_cv;         /* Condition variable for a full channel. */
};

static void* _hlt_channel_read_item(hlt_channel* ch);

void hlt_channel_dtor(hlt_type_info* ti, hlt_channel* c)
{
    while ( c->size ) {
        void* item = _hlt_channel_read_item(c);
        GC_DTOR_GENERIC(item, c->type);
    }

    hlt_channel_chunk* rc = c->rc;

    while ( rc ) {
        hlt_channel_chunk* next = rc->next;
        hlt_free(rc->data);
        hlt_free(rc);
        rc = next;
    }

    pthread_mutex_destroy(&c->mutex);
    pthread_cond_destroy(&c->empty_cv);
    pthread_cond_destroy(&c->full_cv);
}

static hlt_channel_chunk* _hlt_chunk_create(size_t capacity, int16_t item_size, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_channel_chunk *chunk = hlt_malloc(sizeof(hlt_channel_chunk));
    if ( ! chunk ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    chunk->capacity = capacity;
    chunk->wcnt = chunk->rcnt = 0;

    chunk->data = hlt_malloc(capacity * item_size);
    if ( ! chunk->data ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    chunk->next = 0;

    return chunk;
}

// Internal helper function performing a read operation.
static inline void* _hlt_channel_read_item(hlt_channel* ch)
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
static inline int _hlt_channel_write_item(hlt_channel* ch, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ch->wc->wcnt == ch->wc->capacity ) {
        if ( ch->capacity )
            while ( ch->chunk_cap > ch->capacity - ch->size )
                ch->chunk_cap /= 2;
        else if ( ch->chunk_cap < MAX_CHUNK_SIZE )
            ch->chunk_cap *= 2;

        ch->wc->next = _hlt_chunk_create(ch->chunk_cap, ch->type->size, excpt, ctx);
        if ( ! ch->wc->next ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return 1;
        }

        ch->wc = ch->wc->next;
        ch->tail = ch->wc->data;
    }

    memcpy(ch->tail, data, ch->type->size);
    GC_CCTOR_GENERIC(ch->tail, ch->type);

    ++ch->wc->wcnt;

    ch->tail += ch->type->size;
    ++ch->size;

    return 0;
}

hlt_channel* hlt_channel_new(const hlt_type_info* item_type, hlt_channel_capacity capacity, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_channel *ch = GC_NEW(hlt_channel);
    if ( ! ch ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    ch->type = item_type;
    ch->capacity = capacity;
    ch->size = 0;

    ch->chunk_cap = INITIAL_CHUNK_SIZE;
    ch->rc = ch->wc = _hlt_chunk_create(ch->chunk_cap, ch->type->size, excpt, ctx);
    if ( ! ch->rc ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    ch->head = ch->tail = ch->rc->data;

    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->empty_cv, NULL);
    pthread_cond_init(&ch->full_cv, NULL);

    return ch;
}

void hlt_channel_write(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pthread_mutex_lock(&ch->mutex);

    if ( _hlt_channel_write_item(ch, data, excpt, ctx) )
        goto unlock_exit;

    pthread_cond_signal(&ch->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return;
}

void hlt_channel_write_try(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pthread_mutex_lock(&ch->mutex);

    if ( ch->capacity && ch->size == ch->capacity ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        goto unlock_exit;
    }

    if ( _hlt_channel_write_item(ch, data, excpt, ctx) )
        goto unlock_exit;

    pthread_cond_signal(&ch->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return;
}

void* hlt_channel_read(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pthread_mutex_lock(&ch->mutex);

    while ( ch->size == 0 )
        pthread_cond_wait(&ch->empty_cv, &ch->mutex);

    void *item = _hlt_channel_read_item(ch);

    pthread_cond_signal(&ch->full_cv);

    pthread_mutex_unlock(&ch->mutex);
    return item;
}

void* hlt_channel_read_try(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pthread_mutex_lock(&ch->mutex);

    void *item = 0;

    if ( ch->size == 0 ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        goto unlock_exit;
    }

    item = _hlt_channel_read_item(ch);

    pthread_cond_signal(&ch->full_cv);

unlock_exit:
    pthread_mutex_unlock(&ch->mutex);
    return item;
}

hlt_channel_capacity hlt_channel_size(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return ch->size;
}

hlt_string hlt_channel_to_string(const hlt_type_info* type, void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_CHANNEL);
    assert(type->num_params == 2);

    hlt_channel* channel = *((hlt_channel **)obj);

    if ( ! channel )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("channel<", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(",", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);

    hlt_string s = prefix;

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    int i;
    for ( i = 0; i < type->num_params; i++ ) {
        hlt_string t;

        if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj, 0, excpt, ctx);
            if ( *excpt )
                return 0;
        }
        else
            t = hlt_string_from_asciiz(types[i]->tag, excpt, ctx);

        s = hlt_string_concat_and_unref(s, t, excpt, ctx);

        if ( i < type->num_params - 1 ) {
            GC_CCTOR(separator, hlt_string);
            s = hlt_string_concat_and_unref(s, separator, excpt, ctx);
        }
    }

    return hlt_string_concat_and_unref(s, postfix, excpt, ctx);
}
