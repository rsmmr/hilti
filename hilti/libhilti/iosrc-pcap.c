// $Id$

#include "hilti.h"
#include "iosrc.h"

#include <pcap.h>

static hlt_string_constant PREFIX = { 11, "error with " };
static hlt_string_constant COLON = { 3, ": " };

static void _raise_error(hlt_iosrc_pcap* src, const char* errbuf, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* err = errbuf ? errbuf : pcap_geterr(src->handle);

    hlt_string msg = hlt_string_concat(&PREFIX, src->iface, excpt, ctx);
    msg = hlt_string_concat(msg, &COLON, excpt, ctx);
    msg = hlt_string_concat(msg, hlt_string_from_asciiz(err, excpt, ctx), excpt, ctx);

    hlt_set_exception(excpt, &hlt_exception_iosrc_error, msg);
}

static void _strip_link_layer(hlt_iosrc_pcap* src, const char** pkt, int* caplen, int datalink, hlt_exception** excpt, hlt_execution_context* ctx)
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
        _raise_error(src, "unsupported link layer type", excpt, ctx);
        return;
    }

    if ( *caplen < hdr_size ) {
        _raise_error(src, "partially captured packet", excpt, ctx);
        return;
    }

    *pkt += hdr_size;
    *caplen -= hdr_size;
}

static hlt_string_constant STR_PREFIX = { 8, "<pcap source " };
static hlt_string_constant STR_POSTFIX = { 8, ">" };

hlt_string hlt_iosrc_pcap_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_iosrc_pcap* src = *((const hlt_iosrc_pcap**)obj);

    hlt_string str = hlt_string_concat(&STR_PREFIX, src->iface, excpt, ctx);
    return hlt_string_concat(str, &STR_POSTFIX, excpt, ctx);
}

hlt_iosrc_pcap* hlt_iosrc_pcap_new_live(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iosrc_pcap* src = (hlt_iosrc_pcap*) hlt_gc_malloc_non_atomic(sizeof(hlt_iosrc_pcap));
    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    src->type = Hilti_IOSrc_PcapLive;
    src->iface = interface;

    const char* iface = hlt_string_to_native(interface, excpt, ctx);
    if ( *excpt )
        return 0;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *p = pcap_create(iface, errbuf);
    if ( ! p ) {
        _raise_error(src, errbuf, excpt, ctx);
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
    _raise_error(src, 0, excpt, ctx);
    pcap_close(src->handle);
    return 0;
}

hlt_iosrc_pcap* hlt_iosrc_pcap_new_offline(hlt_string interface, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iosrc_pcap* src = (hlt_iosrc_pcap*) hlt_gc_malloc_non_atomic(sizeof(hlt_iosrc_pcap));
    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    src->type = Hilti_IOSrc_PcapOffline;
    src->iface = interface;

    const char* iface = hlt_string_to_native(interface, excpt, ctx);
    if ( *excpt )
        return 0;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *p = pcap_open_offline(iface, errbuf);
    if ( ! p ) {
        _raise_error(src, errbuf, excpt, ctx);
        return 0;
    }

    src->handle = p;
    return src;
}

hlt_packet hlt_iosrc_pcap_read_try(hlt_iosrc_pcap* src, int8_t keep_link_layer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_packet result = { 0.0, NULL };

    if ( ! src->handle ) {
        _raise_error(src, "already closed", excpt, ctx);
        return result;
    }

    struct pcap_pkthdr* hdr;
    const u_char* data;

    int rc = pcap_next_ex(src->handle, &hdr, &data);
    int caplen = hdr->caplen;

    if ( rc > 0 ) {
        // Got a packet.
        if ( ! keep_link_layer ) {
            _strip_link_layer(src, (const char**)&data, &caplen, pcap_datalink(src->handle), excpt, ctx);
            if ( *excpt )
                return result;
        }

        // We need to copy it to make sure it remains valid.
        void *copy = hlt_gc_malloc_atomic(caplen);
        memcpy(copy, data, caplen);
        hlt_bytes* pkt = hlt_bytes_new_from_data(copy, caplen, excpt, ctx);
        if ( *excpt )
            return result;

        // Build the result tuple.
        result.t = hlt_time_value(hdr->ts.tv_sec, hdr->ts.tv_usec * 1000);
        result.data = pkt;
        return result;
    }

    if ( rc == -2 ) {
        // No more packets.
        return result;
    }

    if ( rc < 0 ) {
        // Error.
        _raise_error(src, 0, excpt, ctx);
        pcap_close(src->handle);
        src->handle = 0;
        return result;
    }

    // Don't think we can get here when reading from a trace ...
    assert(! hlt_enum_equal(src->type, Hilti_IOSrc_PcapOffline, excpt, ctx));

    // No packet this time.
    hlt_set_exception(excpt, &hlt_exception_would_block, 0);
    return result;
}

void hlt_iosrc_pcap_close(hlt_iosrc_pcap* src, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pcap_close(src->handle);
    src->handle = 0;
}

