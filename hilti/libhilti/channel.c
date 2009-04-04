/* $Id$
 * 
 * Support functions HILTI's channel data type.
 * 
 */

#include <string.h>

#include "hilti_intern.h"

#define DEFAULT_CHANNEL_SIZE (1<<12)

static const char* channel_name = "channel";
static const __hlt_string prefix = { 1, "<" };
static const __hlt_string postfix = { 1, ">" };

const __hlt_string* __hlt_channel_to_string(const __hlt_type_info* type, void* (*obj[]), int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_CHANNEL);
    assert(type->num_params == 1);

    const __hlt_string *s = __hlt_string_from_asciiz(channel_name, excpt);
    s = __hlt_string_concat(s, &prefix, excpt);
    if ( *excpt )
        return 0;

    __hlt_type_info** types = (__hlt_type_info**) &type->type_params;

    const __hlt_string *t;
    if ( types[0]->to_string ) {
        t = (types[0]->to_string)(types[0], (*obj)[0], 0, excpt);
            if ( *excpt )
                return 0;
    }
    else
        // No format function.
        t = __hlt_string_from_asciiz(types[0]->tag, excpt);

    s = __hlt_string_concat(s, t, excpt);
    if ( *excpt )
        return 0;

    return __hlt_string_concat(s, &postfix, excpt);
}

__hlt_channel* __hlt_channel_new(const __hlt_type_info* type, __hlt_exception* excpt)
{
    __hlt_channel *ch = __hlt_gc_malloc_atomic(sizeof(__hlt_channel));
    if ( ! ch ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    ch->type = type;
    ch->size = DEFAULT_CHANNEL_SIZE / type->size;    /* FIXME: Specify as type parameter. */

    ch->buffer = malloc(ch->size * type->size);
    if ( ! ch->buffer ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    ch->head = 0;
    ch->tail = ch->size - 1;
    ch->node_count = 0;

    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->empty_cv, NULL);
    pthread_cond_init(&ch->full_cv, NULL);

    return ch;
}

void __hlt_channel_destroy(__hlt_channel* ch, __hlt_exception* excpt)
{
    free(ch->buffer);

    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->empty_cv);
    pthread_cond_destroy(&ch->full_cv);
}

void __hlt_channel_write(__hlt_channel* ch, const __hlt_type_info* type, void* data, __hlt_exception* excpt)
{ 
    pthread_mutex_lock(&ch->mutex);

    while (ch->node_count == ch->size)
        pthread_cond_wait(&ch->full_cv, &ch->mutex);

    ch->tail = (ch->tail + 1) % ch->size;
    memcpy(ch->buffer + ch->tail * ch->type->size, data, ch->type->size);
    ++ch->node_count;

    pthread_mutex_unlock(&ch->mutex);

    pthread_cond_signal(&ch->empty_cv);
}

void* __hlt_channel_read(__hlt_channel* ch, __hlt_exception* excpt)
{
    pthread_mutex_lock(&ch->mutex);

    while ( ch->node_count == 0 )
        pthread_cond_wait(&ch->empty_cv, &ch->mutex);

    void *node = __hlt_gc_malloc_atomic(ch->type->size);
    memcpy(node, ch->buffer + ch->head * ch->type->size, ch->type->size);
    ch->head = (ch->head + 1) % ch->size;
    --ch->node_count;
    
    pthread_mutex_unlock(&ch->mutex);

    pthread_cond_signal(&ch->full_cv);

    return node;
}
