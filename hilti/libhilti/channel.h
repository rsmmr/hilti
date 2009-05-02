/* $Id$
 * 
 * API for HILTI's channel data type.
 * 
 */

#ifndef HILTI_CHANNEL_H
#define HILTI_CHANNEL_H

#include "hilti_intern.h"

typedef int64_t __hlt_channel_size;
typedef struct __hlt_channel __hlt_channel;

// A gc'ed chunk of memory used by the channel.
typedef struct __hlt_channel_chunk __hlt_channel_chunk;

    // %doc-hlt_channel-start
struct __hlt_channel {
    const __hlt_type_info* type;        /* Type information of the channel's data type. */
    __hlt_channel_size capacity;        /* Maximum number of channel items. */
    volatile __hlt_channel_size size;   /* Current number of channel items. */

    size_t chunk_cap;                   /* Chunk capacity of the next chunk. */
    __hlt_channel_chunk* rc;            /* Pointer to the reader chunk. */
    __hlt_channel_chunk* wc;            /* Pointer to the writer chunk. */
    void* head;                         /* Pointer to the next item to read. */
    void* tail;                         /* Pointer to the first empty slot for writing. */

    pthread_mutex_t mutex;              /* Synchronizes access to the channel. */
    pthread_cond_t empty_cv;            /* Condition variable for an empty channel. */
    pthread_cond_t full_cv;             /* Condition variable for a full channel. */
};
    // %doc-hlt_channel-end

// Returns a readable representation of a channel.
extern const __hlt_string* __hlt_channel_to_string(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* excpt);

// Creates a new channel.
extern __hlt_channel* __hlt_channel_new(const __hlt_type_info* type, __hlt_exception* excpt);

// Deletes a channel.
extern void __hlt_channel_destroy(__hlt_channel* ch, __hlt_exception* excpt);

// Write an item into a channel. If the channel is full, the function blocks
// until an item is read from the channel.
extern void __hlt_channel_write(__hlt_channel* ch, const __hlt_type_info* type, void* data, __hlt_exception* excpt);

// Try to write an item into a channel. If the channel is full, an exception is
// thrown indicating that the channel was full.
extern void __hlt_channel_try_write(__hlt_channel* ch, const __hlt_type_info* type, void* data, __hlt_exception* excpt);

// Read an item from a channel. If the channel is empty, the function blocks
// until an item is written to the channel.
extern void* __hlt_channel_read(__hlt_channel* ch, __hlt_exception* excpt);

// Try to read an element from the channel. If the channel is empty, an
// exception is thrown indicating that the channel was empty. 
extern void* __hlt_channel_try_read(__hlt_channel* ch, __hlt_exception* excpt);

// Returns the current channel size, i.e., the number of items in the channel.
extern __hlt_channel_size __hlt_channel_get_size(__hlt_channel* ch, __hlt_exception* excpt);

#endif
