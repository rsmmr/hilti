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
#include "clone.h"

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

// State shared across channel instances originating from the same root
// value.
typedef struct {
    const hlt_type_info* type;      /* Type information of the channel's data type. */
    hlt_channel_capacity capacity;      /* Maximum number of channel items. */
    volatile hlt_channel_capacity size; /* Current number of channel items. */

    size_t chunk_cap;               /* Chunk capacity of the next chunk. */
    hlt_channel_chunk* rc;          /* Pointer to the reader chunk. */
    hlt_channel_chunk* wc;          /* Pointer to the writer chunk. */
    void* head;                     /* Pointer to the next item to read. */
    void* tail;                     /* Pointer to the first empty slot for writing. */

    uint64_t ref_cnt;               /* Self-managed ref count for the shared state. */

    pthread_mutex_t mutex;          /* Synchronizes access to the channel. */
    pthread_cond_t empty_cv;        /* Condition variable for an empty channel. */
    pthread_cond_t full_cv;         /* Condition variable for a full channel. */
} __hlt_channel_shared;

struct __hlt_channel {
    __hlt_gchdr __gchdr;            /* Header for memory management. */
    __hlt_channel_shared* shared;   /* Shared implementation state. */
};

static void* _hlt_channel_read_item(hlt_channel* ch);

void hlt_channel_dtor(hlt_type_info* ti, hlt_channel* ch)
{
    __hlt_channel_shared* shared = ch->shared;

    pthread_mutex_lock(&shared->mutex);

    if ( --shared->ref_cnt > 0 ) {
        pthread_mutex_unlock(&shared->mutex);
        return;
    }

    // Delete, we're the last one holding a reference to the shared state.

    pthread_mutex_unlock(&shared->mutex);

    while ( shared->size ) {
        void* item = _hlt_channel_read_item(ch);
        GC_DTOR_GENERIC(item, shared->type);
    }

    hlt_channel_chunk* rc = shared->rc;

    while ( rc ) {
        hlt_channel_chunk* next = rc->next;
        hlt_free(rc->data);
        hlt_free(rc);
        rc = next;
    }

    pthread_mutex_destroy(&shared->mutex);
    pthread_cond_destroy(&shared->empty_cv);
    pthread_cond_destroy(&shared->full_cv);

    hlt_free(shared);
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
    __hlt_channel_shared* shared = ch->shared;

    if ( shared->rc->rcnt == shared->rc->capacity ) {
        assert(shared->rc->next);

        shared->rc = shared->rc->next;
        shared->head = shared->rc->data;
    }

    void* item = shared->head;
    ++shared->rc->rcnt;

    shared->head += shared->type->size;
    --shared->size;

    return item;
}

// Internal helper function performing a write operation.
static inline int _hlt_channel_write_item(hlt_channel* ch, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    if ( shared->wc->wcnt == shared->wc->capacity ) {
        if ( shared->capacity )
            while ( shared->chunk_cap > shared->capacity - shared->size )
                shared->chunk_cap /= 2;
        else if ( shared->chunk_cap < MAX_CHUNK_SIZE )
            shared->chunk_cap *= 2;

        shared->wc->next = _hlt_chunk_create(shared->chunk_cap, shared->type->size, excpt, ctx);
        if ( ! shared->wc->next ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return 1;
        }

        shared->wc = shared->wc->next;
        shared->tail = shared->wc->data;
    }

#ifdef HLT_DEEP_COPY_VALUES_ACROSS_THREADS
    hlt_clone_deep(shared->tail, shared->type, data, excpt, ctx);
#else
    memcpy(shared->tail, data, shared->type->size);
    GC_CCTOR_GENERIC(shared->tail, shared->type);
#endif

    ++shared->wc->wcnt;

    shared->tail += shared->type->size;
    ++shared->size;

    return 0;
}

void* hlt_channel_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GC_NEW(hlt_channel);
}

void hlt_channel_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_channel* src = *(hlt_channel**)srcp;
    hlt_channel* dst = *(hlt_channel**)dstp;

    __hlt_channel_shared* shared = src->shared;

    pthread_mutex_lock(&shared->mutex);

    ++shared->ref_cnt;
    dst->shared = shared;

    pthread_mutex_unlock(&shared->mutex);
}

hlt_channel* hlt_channel_new(const hlt_type_info* item_type, hlt_channel_capacity capacity, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_channel *ch = GC_NEW(hlt_channel);
    if ( ! ch ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    __hlt_channel_shared* shared = hlt_malloc(sizeof(__hlt_channel_shared));
    ch->shared = shared;

    shared->ref_cnt = 1;
    shared->type = item_type;
    shared->capacity = capacity;
    shared->size = 0;

    shared->chunk_cap = INITIAL_CHUNK_SIZE;
    shared->rc = shared->wc = _hlt_chunk_create(shared->chunk_cap, shared->type->size, excpt, ctx);
    if ( ! shared->rc ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    shared->head = shared->tail = shared->rc->data;

    pthread_mutex_init(&shared->mutex, NULL);
    pthread_cond_init(&shared->empty_cv, NULL);
    pthread_cond_init(&shared->full_cv, NULL);

    return ch;
}

void hlt_channel_write(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    pthread_mutex_lock(&shared->mutex);

    if ( _hlt_channel_write_item(ch, data, excpt, ctx) )
        goto unlock_exit;

    pthread_cond_signal(&shared->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&shared->mutex);
    return;
}

void hlt_channel_write_try(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    pthread_mutex_lock(&shared->mutex);

    if ( shared->capacity && shared->size == shared->capacity ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        goto unlock_exit;
    }

    if ( _hlt_channel_write_item(ch, data, excpt, ctx) )
        goto unlock_exit;

    pthread_cond_signal(&shared->empty_cv);

unlock_exit:
    pthread_mutex_unlock(&shared->mutex);
    return;
}

void* hlt_channel_read(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    pthread_mutex_lock(&shared->mutex);

    while ( shared->size == 0 )
        pthread_cond_wait(&shared->empty_cv, &shared->mutex);

    void *item = _hlt_channel_read_item(ch);

    pthread_cond_signal(&shared->full_cv);

    pthread_mutex_unlock(&shared->mutex);
    return item;
}

void* hlt_channel_read_try(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    pthread_mutex_lock(&shared->mutex);

    void *item = 0;

    if ( shared->size == 0 ) {
        hlt_set_exception(excpt, &hlt_exception_would_block, 0);
        goto unlock_exit;
    }

    item = _hlt_channel_read_item(ch);

    pthread_cond_signal(&shared->full_cv);

unlock_exit:
    pthread_mutex_unlock(&shared->mutex);
    return item;
}

hlt_channel_capacity hlt_channel_size(hlt_channel* ch, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_channel_shared* shared = ch->shared;

    return shared->size;
}

hlt_string hlt_channel_to_string(const hlt_type_info* type, void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
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
        hlt_string t = hlt_object_to_string(types[i], obj, options, seen, excpt, ctx);
        s = hlt_string_concat_and_unref(s, t, excpt, ctx);

        if ( i < type->num_params - 1 ) {
            GC_CCTOR(separator, hlt_string);
            s = hlt_string_concat_and_unref(s, separator, excpt, ctx);
        }
    }

    return hlt_string_concat_and_unref(s, postfix, excpt, ctx);
}
