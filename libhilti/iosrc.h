// $Id$
//
// $Id$
//
// I/O source provide a general abstraction for dealing with high-volume
// external input/output (well, input only at the moment). Each I/O source
// implements a similar interface.
//
// For now, we online provide one implementation for libpcap input. More to
// come.
//
// Todo: This interface is quite preliminary; we'll need to refine it when we
// add more sources. For example, we probably want some kind of dispatcher
// functions that select the right driver based on the source's type.

/// \addtogroup iosrc
/// @{

#ifndef HILTI_IOSRC_H
#define HILTI_IOSRC_H

#include "types.h"
#include "enum.h"
#include "time_.h"
#include "bytes.h"

/// The type of an IOSource as one of the Hilti::IOSrc constants.
typedef hlt_enum hlt_iosrc_type;

struct __hlt_iosrc {
    __hlt_gchdr __gchdr;    // Header for memory management.
    hlt_iosrc_type type;  // Hilti_PktSrc_PcapLive or Hilti_PktSrc_PcapOffline.
    hlt_string iface;     // The name of the interface.
    void* handle;         // A kind-specific handle.
};

/// tuple<time, ref<bytes>>
struct __hlt_packet {
    hlt_time t;
    hlt_bytes* data;
};

typedef struct __hlt_packet hlt_packet;

/// Type for packet sources.
typedef struct __hlt_iosrc hlt_iosrc;

/// Creates a new PCAP packet source for live input.
///
/// interface: The name of the interface. The interface will be put into
/// promiscious mode.
///
/// Raises: IOError if there is a problem opening the the interface for monitoring.
extern hlt_iosrc* hlt_iosrc_new_live(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx);

/// Creates a new PCAP packet source for offline input.
///
/// interface: The name of the trace file.
///
/// Raises: IOError if there is a problem opening the the interface for monitoring.
extern hlt_iosrc* hlt_iosrc_new_offline(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx);

/// Attempts to reads a packet from a PCAP source. If no packet is currently
/// available, raises a WouldBlock exception if there might be one at a later
/// time. If the source is permanently exhausted, returns a null pointer (see
/// below).
///
/// src: The packet source.
///
/// keep_link_layer: If not true, any link layer headers are stripped.
///
/// Returns: A tuple <hlt_time, hlt_bytes*> in which the time is the
/// packet's timestamp, and the ``bytes`` object is the packet's content. If
/// the source is permanenly exhausted, the ``bytes`` pointer will be null.
///
/// Raises: IOError if there are any errors other than those described
/// above, including encountering an unsupported link-layer header if
/// *keep_link_layer* is disabled.
extern hlt_packet hlt_iosrc_read_try(hlt_iosrc* src, int8_t keep_link_layer, hlt_exception** excpt, hlt_execution_context* ctx);

/// Closes a live PCAP packet source. Any attempt to read further packets
/// will result in an IOSrcError exception.
///
/// src: The packet source to close.
extern void hlt_iosrc_close(hlt_iosrc* src, hlt_exception** excpt, hlt_execution_context* ctx);

#endif

/// @}
