// $Id$
//
// I/O source providing access to libpcap data.

/// \addtogroup iosrc-pcap
/// @{

#ifndef HILTI_IOSRC_PCAP_H
#define HILTI_IOSRC_PCAP_H

struct __hlt_iosrc_pcap {
    hlt_iosrc_type type;  // Hilti_PktSrc_PcapLive or Hilti_PktSrc_PcapOffline.
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
typedef struct __hlt_iosrc_pcap hlt_iosrc_pcap;

/// Creates a new PCAP packet source for live input.
///
/// interface: The name of the interface. The interface will be put into
/// promiscious mode.
///
/// Raises: IOSrcError if there is a problem opening the the interface for monitoring.
extern hlt_iosrc_pcap* hlt_iosrc_pcap_new_live(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx);

/// Creates a new PCAP packet source for offline input.
///
/// interface: The name of the trace file.
///
/// Raises: IOSrcError if there is a problem opening the the interface for monitoring.
extern hlt_iosrc_pcap* hlt_iosrc_pcap_new_offline(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx);

/// Attempts to reads a packet from a PCAP source. If no packet is currently
/// available, raises a WouldBlock exception if there might be one at a later
/// time. If the source is permanently exhausted, returns a null pointer (see
/// below).
///
/// src: The packet source.
///
/// keep_link_layer: If not true, any link layer headers are stripped.
///
/// Returns: A tuple <double, hlt_bytes*> in which the ``double`` is the
/// packet's timestamp, and the ``bytes`` object is the packet's content. If
/// the source is permanenly exhausted, the ``bytes`` pointer will be null.
///
/// Raises: IOSrcError if there are any errors other than those described
/// above, including encountering an unsupported link-layer header if
/// *keep_link_layer* is disabled.
extern hlt_packet hlt_iosrc_pcap_read_try(hlt_iosrc_pcap* src, int8_t keep_link_layer, hlt_exception** excpt, hlt_execution_context* ctx);

/// Closes a live PCAP packet source. Any attempt to read further packets
/// will result in an IOSrcError exception.
///
/// src: The packet source to close.
extern void hlt_iosrc_pcap_close(hlt_iosrc_pcap* src, hlt_exception** excpt, hlt_execution_context* ctx);

#endif

/// @}
