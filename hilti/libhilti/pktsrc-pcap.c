// $Id$

#include <pcap.h>

#include "pktsrc.h"

extern const hlt_type_info hlt_type_info_tuple_double_ref_bytes;

static hlt_string_constant PREFIX = { 11, "error with " };
static hlt_string_constant COLON = { 3, ": " };

static void _raise_error(hlt_pktsrc* src, const char* errbuf, hlt_exception** excpt)
{
    const char* err = errbuf ? errbuf : pcap_geterr(src->handle);
    
    hlt_string msg = hlt_string_concat(&PREFIX, src->iface, excpt);
    msg = hlt_string_concat(msg, &COLON, excpt);
    msg = hlt_string_concat(msg, hlt_string_from_asciiz(err, excpt), excpt);
                            
    hlt_set_exception(excpt, &hlt_exception_pktsrc_error, msg);
}

static void _strip_link_layer(hlt_pktsrc* src, const char** pkt, int* caplen, int datalink, hlt_exception** excpt)
{
    int hdr_size = 0;
    
    switch ( datalink ) {
      case DLT_NULL:
        hdr_size = 4; 
        break;
            
      case DLT_EN10MB:
        hdr_size = 14; 
        break;
            
      case DLT_FDDI:
        hdr_size = 21; 
        break;
            
      case DLT_LINUX_SLL:
        hdr_size = 16; 
        break;
        
      case DLT_RAW:
        hdr_size = 0; 
        break;
        
      default:
        _raise_error(src, "unsupported link layer type", excpt);
        return;
    }
     
    if ( *caplen < hdr_size ) {
        _raise_error(src, "partially captured packet", excpt);
        return;
    }
    
    *pkt += hdr_size;
    *caplen -= hdr_size;
}

static hlt_packet _pcap_next_packet(hlt_pktsrc* src, int8_t block, int8_t keep_link_layer, hlt_exception** excpt)
{
    hlt_packet result = { 0.0, NULL };
    
    if ( ! src->handle ) {
        _raise_error(src, "already closed", excpt);
        return result;
    }
    
    struct pcap_pkthdr* hdr;
    const u_char* data;
    
    while ( true ) {
        int rc = pcap_next_ex(src->handle, &hdr, &data);
        int caplen = hdr->caplen;
        
        if ( rc > 0 ) {
            // Got a packet. 
            if ( ! keep_link_layer ) {
                _strip_link_layer(src, (const char**)&data, &caplen, pcap_datalink(src->handle), excpt);
                if ( *excpt )
                    return result;
            }
             
            // We need to copy it to make sure it remains valid. 
            void *copy = hlt_gc_malloc_atomic(caplen);
            memcpy(copy, data, caplen);
            hlt_bytes* pkt = hlt_bytes_new_from_data(copy, caplen, excpt);
            if ( *excpt )
                return result;
            
            // Build the result tuple.
            result.t = hdr->ts.tv_sec + (double)(hdr->ts.tv_usec) / 1e6;
            result.data = pkt;
            return result;
        }

        if ( rc == -2 ) {
            // No more packets.
            return result;
        }
        
        if ( rc < 0 ) {
            // Error.
            _raise_error(src, 0, excpt);
            pcap_close(src->handle);
            src->handle = 0;
            return result;
        }
        
        // Don't think we can get here when reading from a trace ...
        assert(src->kind != Hilti_PktSrc_PcapOffline);
        
        if ( ! block )
            // No packet this time.
            return result;
        
    }
}

static hlt_string_constant STR_PREFIX = { 8, "<pktsrc " };
static hlt_string_constant STR_POSTFIX = { 8, ">" };

hlt_string hlt_pktsrc_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt)
{
    const hlt_pktsrc* src = *((const hlt_pktsrc**)obj);

    hlt_string str = hlt_string_concat(&STR_PREFIX, src->iface, excpt);
    return hlt_string_concat(str, &STR_POSTFIX, excpt);
}

///////// Live PCAP input.

hlt_pktsrc* hlt_pktsrc_new_pcap_live(hlt_string interface, hlt_exception** excpt)
{
    hlt_pktsrc* src = (hlt_pktsrc*) hlt_gc_malloc_non_atomic(sizeof(hlt_pktsrc));
    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    src->kind = Hilti_PktSrc_PcapLive;
    src->iface = interface;
    
    const char* iface = hlt_string_to_native(interface, excpt);
    if ( *excpt )
        return 0;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *p = pcap_create(iface, errbuf);
    if ( ! p ) {
        _raise_error(src, errbuf, excpt);
        return 0;
    }

    src->handle = p;
    
    if ( pcap_set_snaplen(p, 65535) != 0 )
        goto error;
    
    if ( pcap_set_promisc(p, 1) != 0 )
        goto error;
    
    if ( pcap_set_timeout(p, 1) != 0 )
        goto error;
    
    if ( pcap_activate(p) != 0 )
        goto error;
    
    // Up and running.
    return src;
    
error:    
    _raise_error(src, 0, excpt);
    pcap_close(src->handle);
    return 0;
}

hlt_packet hlt_pktsrc_read_pcap_live(hlt_pktsrc* src, int8_t block, int8_t keep_link_layer, hlt_exception** excpt)
{
    assert(src->kind == Hilti_PktSrc_PcapLive);
    return _pcap_next_packet(src, block, keep_link_layer, excpt);
}

void hlt_pktsrc_close_pcap_live(hlt_pktsrc* src, hlt_exception** excpt)
{
    assert(src->kind == Hilti_PktSrc_PcapLive);
    
    pcap_close(src->handle);
    src->handle = 0;
}

///////// Offline PCAP input.

hlt_pktsrc* hlt_pktsrc_new_pcap_offline(hlt_string interface, hlt_exception** excpt)
{
    hlt_pktsrc* src = (hlt_pktsrc*) hlt_gc_malloc_non_atomic(sizeof(hlt_pktsrc));
    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    src->kind = Hilti_PktSrc_PcapOffline;
    src->iface = interface;
    
    const char* iface = hlt_string_to_native(interface, excpt);
    if ( *excpt )
        return 0;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *p = pcap_open_offline(iface, errbuf);
    if ( ! p ) {
        _raise_error(src, errbuf, excpt);
        return 0;
    }

    src->handle = p;
    return src;
}

hlt_packet hlt_pktsrc_read_pcap_offline(hlt_pktsrc* src, int8_t block, int8_t keep_link_layer, hlt_exception** excpt)
{
    assert(src->kind == Hilti_PktSrc_PcapOffline);
    return _pcap_next_packet(src, block, keep_link_layer, excpt);
}

void hlt_pktsrc_close_pcap_offline(hlt_pktsrc* src, hlt_exception** excpt)
{
    assert(src->kind == Hilti_PktSrc_PcapOffline);
    
    pcap_close(src->handle);
    src->handle = 0;
    return;
}

