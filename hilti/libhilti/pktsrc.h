// $Id$
//
// Packet source interface.
// 
// Note that at the moment we only have the API for when the kind of source
// is already known. We should a set of dispatcher functions that look at the
// kind field in hlt_pktsrc and then call the corresponding backend function
// (hiltic does that during compilation based on type information).

/// \addtogroup pktsrc
/// @{

#ifndef HILTI_PKTSRC_H
#define HILTI_PKTSRC_H

#include "hilti.h"

struct __hlt_pktsrc {
    int16_t kind;         // One of the hlt_pktsrc_kind values.
    hlt_string iface;     // The name of the interface. 
    void* handle;         // A kind-specific handle. 
};

/// tuple<double, ref<bytes>>
struct __hlt_packet {
    double t;
    hlt_bytes* data;
};

typedef struct __hlt_packet hlt_packet;

/// Type for packet sources.
typedef struct __hlt_pktsrc hlt_pktsrc;

///////// Live PCAP input.

/// Creates a new PCAP packet source for live input.
/// 
/// interface: The name of the interface. The interface will be put into
/// promiscious mode. 
/// 
/// Raises: PktSrcError if there is a problem opening the the interface for monitoring. 
extern hlt_pktsrc* hlt_pktsrc_new_pcap_live(hlt_string interface, hlt_exception** excpt);

/// Reads the next packet from a live PCAP source. 
/// 
/// src: The packet source.
/// 
/// block: If true, the function blocks until a packet is available. 
/// 
/// keep_link_layer: If not true, any link layer headers are stripped. 
/// 
/// Returns: A tuple <double, hlt_bytes*> in which the ``double`` is the
/// packet's timestamp, and the ``bytes`` object is the packet's content.
/// Returns NULL as the bytes object if *block* is false and no packet is
/// currently available, or if the source is exhausted.
/// 
/// Raises: PktSrcError if there are any other errors, including encountering
/// an unsupported link-layer header if *keep_link_layer* is disabled. 
extern hlt_packet hlt_pktsrc_read_pcap_live(hlt_pktsrc* src, int8_t block, int8_t keep_link_layer, hlt_exception** excpt);

/// Closes a live PCAP packet source. Any attempt to read further packets
/// will result in a PktSrcError exception. 
/// 
/// src: The packet source to close. 
extern void hlt_pktsrc_close_pcap_live(hlt_pktsrc* src, hlt_exception** excpt);

///////// Offline PCAP input.

/// Creates a new PCAP packet source for offline input.
/// 
/// interface: The name of the interface. The interface will be put into
/// promiscious mode. 
/// 
/// Raises: PktSrcError if there is a problem opening the the interface for monitoring. 
extern hlt_pktsrc* hlt_pktsrc_new_pcap_offline(hlt_string interface, hlt_exception** excpt);

/// Reads the next packet from a offline PCAP source. 
/// 
/// src: The packet source.
/// 
/// block: If true, the function blocks until a packet is available. 
/// 
/// keep_link_layer: If true, any link layer headers are removed. 
/// 
/// Returns: A tuple <double, hlt_bytes*> in which the ``double`` is the
/// packet's timestamp, and the ``bytes`` object is the packet's content.
/// Returns NULL as the bytes object if *block* is false and no packet
/// is currently available, or if the source is exhausted.
/// 
/// Raises: PktSrcError if there are any other errors, including encountering
/// an unsupported link-layer header if *keep_link_layer* is disabled. 
extern hlt_packet hlt_pktsrc_read_pcap_offline(hlt_pktsrc* src, int8_t block, int8_t keep_link_layer, hlt_exception** excpt);

/// Closes an offline PCAP packet source. Any attempt to read further packets
/// will result in a PktSrcError exception. 
/// 
/// src: The packet source to close. 
extern void hlt_pktsrc_close_pcap_offline(hlt_pktsrc* src, hlt_exception** excpt);

#endif

/// @}
