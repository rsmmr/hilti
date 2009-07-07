/* $Id$
 * 
 * API for HILTI's channel data type.
 * 
 */

#ifndef HILTI_HILTI_CHANNEL_H
#define HILTI_HILTI_CHANNEL_H

#include "exceptions.h"

typedef int64_t hlt_channel_size;
typedef struct hlt_channel hlt_channel;

// A gc'ed chunk of memory used by the channel.
typedef struct hlt_channel_chunk hlt_channel_chunk;

    // %doc-hlt_channel-start
struct hlt_channel {
    const hlt_type_info* type;        /* Type information of the channel's data type. */
    hlt_channel_size capacity;        /* Maximum number of channel items. */
    volatile hlt_channel_size size;   /* Current number of channel items. */

    size_t chunk_cap;                   /* Chunk capacity of the next chunk. */
    hlt_channel_chunk* rc;            /* Pointer to the reader chunk. */
    hlt_channel_chunk* wc;            /* Pointer to the writer chunk. */
    void* head;                         /* Pointer to the next item to read. */
    void* tail;                         /* Pointer to the first empty slot for writing. */

    pthread_mutex_t mutex;              /* Synchronizes access to the channel. */
    pthread_cond_t empty_cv;            /* Condition variable for an empty channel. */
    pthread_cond_t full_cv;             /* Condition variable for a full channel. */
};
    // %doc-hlt_channel-end

/* FIXME: This struct represents the type parameters that are currently only
 * available at the HILTI layer. The compiler should eventually autogenerate
 * such structs.
 */
typedef struct hlt_channel_type_parameters
{
    hlt_type_info* item_type;
    uint64_t capacity;
} hlt_channel_params;


// Returns a readable representation of a channel.
extern hlt_string hlt_channel_to_string(const hlt_type_info* type, void* obj, int32_t options, hlt_exception* excpt);

// Creates a new channel.
extern hlt_channel* hlt_channel_new(const hlt_type_info* type, hlt_exception* excpt);

// Deletes a channel.
extern void hlt_channel_destroy(hlt_channel* ch, hlt_exception* excpt);

// Write an item into a channel. If the channel is full, the function blocks
// until an item is read from the channel.
extern void hlt_channel_write(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception* excpt);

// Try to write an item into a channel. If the channel is full, an exception is
// thrown indicating that the channel was full.
extern void hlt_channel_try_write(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception* excpt);

// Read an item from a channel. If the channel is empty, the function blocks
// until an item is written to the channel.
extern void* hlt_channel_read(hlt_channel* ch, hlt_exception* excpt);

// Try to read an element from the channel. If the channel is empty, an
// exception is thrown indicating that the channel was empty. 
extern void* hlt_channel_try_read(hlt_channel* ch, hlt_exception* excpt);

// Returns the current channel size, i.e., the number of items in the channel.
extern hlt_channel_size hlt_channel_get_size(hlt_channel* ch, hlt_exception* excpt);

#endif
