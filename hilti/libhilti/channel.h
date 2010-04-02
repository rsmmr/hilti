/* $Id$
 * 
 * API for HILTI's channel data type.
 * 
 */

#ifndef HILTI_HILTI_CHANNEL_H
#define HILTI_HILTI_CHANNEL_H

#include "exceptions.h"

/// Type for current size and capacity of a channel.
typedef int64_t hlt_channel_capacity;
typedef struct hlt_channel hlt_channel;

/// Creates a new channel. The channel's capacity can be either bounded or
/// unbounded. 
/// 
/// item_type: The type of the items written into the channel. 
/// 
/// capacity: The maximum capacity of the channel, or zero if unbounded. 
/// 
/// excpt: &
/// 
/// Returns: The new channel.
extern hlt_channel* hlt_channel_new(const hlt_type_info* item_type, hlt_channel_capacity capacity, hlt_exception** excpt);

/// Write an item into a channel. If the channel has already reached its
/// capacity, the function blocks until an item is read from the channel.
/// 
/// ch: The channel to write into. 
/// 
/// data: The item to write. 
/// 
/// excpt: &
/// 
/// Note: When the write blocks, the function does not yield processing in
/// any form (because we can't from a C function).
extern void hlt_channel_write(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt);

/// Attemtps to write an item into a channel. If the channel has already
/// reached its capacity, a WouldBlock exception is thrown.
/// 
/// ch: The channel to write into. 
/// 
/// data: The item to write. 
/// 
/// excpt: &
extern void hlt_channel_write_try(hlt_channel* ch, const hlt_type_info* type, void* data, hlt_exception** excpt);

/// Reads an item from a channel. If the channel is empty, the function
/// blocks until an item is written to the channel.
/// 
/// ch: The channel to read from. 
/// 
/// excpt: &
/// 
/// Returns: A pointer to the read item. 
/// 
/// Note: When the read blocks, the function does not yield processing in any
/// form (because we can't from a C function).
extern void* hlt_channel_read(hlt_channel* ch, hlt_exception** excpt);

/// Attempts to read an item from a channel. If the channel is empty, 
/// a WouldBlock exception is thrown.
/// 
/// ch: The channel to read from. 
/// 
/// excpt: &
/// 
/// Returns: A pointer to the read item. 
extern void* hlt_channel_read_try(hlt_channel* ch, hlt_exception** excpt);

/// Returns the current channel size, i.e., the number of items in the
/// channel.
/// 
/// ch: The channel.
/// 
/// excpt: &
/// 
/// Returns: The channel's current size. 
extern hlt_channel_capacity hlt_channel_size(hlt_channel* ch, hlt_exception** excpt);

#endif
