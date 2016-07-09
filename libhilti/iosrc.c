
#include <pcap.h>

#include "autogen/hilti-hlt.h"
#include "iosrc.h"

typedef struct {
    hlt_iosrc* src;
    hlt_time t;
    hlt_bytes* pkt;
} __hlt_iterator_iosrc;

void hlt_iterator_iosrc_cctor(hlt_type_info* ti, __hlt_iterator_iosrc* i,
                              hlt_execution_context* ctx)
{
    GC_CCTOR(i->src, hlt_iosrc, ctx);
    GC_CCTOR(i->pkt, hlt_bytes, ctx);
}

void hlt_iterator_iosrc_dtor(hlt_type_info* ti, __hlt_iterator_iosrc* i, hlt_execution_context* ctx)
{
    GC_DTOR(i->src, hlt_iosrc, ctx);
    GC_DTOR(i->pkt, hlt_bytes, ctx);
}

static void _raise_error(hlt_iosrc* src, const char* errbuf, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    const char* err = errbuf ? errbuf : pcap_geterr(src->handle);

    hlt_string prefix = hlt_string_from_asciiz("error with ", excpt, ctx);
    hlt_string colon = hlt_string_from_asciiz(": ", excpt, ctx);

    hlt_string msg = hlt_string_concat(prefix, src->iface, excpt, ctx);

    msg = hlt_string_concat(msg, colon, excpt, ctx);
    msg = hlt_string_concat(msg, hlt_string_from_asciiz(err, excpt, ctx), excpt, ctx);

    hlt_set_exception(excpt, &hlt_exception_io_error, msg, ctx);
}

static void _strip_link_layer(hlt_iosrc* src, const char** pkt, int* caplen, int datalink,
                              hlt_exception** excpt, hlt_execution_context* ctx)
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

void hlt_iosrc_dtor(hlt_type_info* ti, hlt_iosrc* c, hlt_execution_context* ctx)
{
    if ( c->handle )
        pcap_close(c->handle);

    GC_CLEAR(c->iface, hlt_string, ctx);
}

hlt_string hlt_iosrc_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                               __hlt_pointer_stack* seen, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    const hlt_iosrc* src = *((const hlt_iosrc**)obj);

    if ( ! src )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<pcap source ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);

    hlt_string str = hlt_string_concat(prefix, src->iface, excpt, ctx);
    str = hlt_string_concat(str, postfix, excpt, ctx);

    return str;
}

hlt_iosrc* hlt_iosrc_new_live(hlt_string interface, hlt_exception** excpt,
                              hlt_execution_context* ctx)
{
    hlt_iosrc* src = GC_NEW(hlt_iosrc, ctx);
    src->type = Hilti_IOSrc_PcapLive;
    src->iface = hlt_string_copy(interface, excpt, ctx);
    GC_CCTOR(src->iface, hlt_string, ctx);

    char* iface = hlt_string_to_native(interface, excpt, ctx);
    if ( hlt_check_exception(excpt) )
        return 0;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* p = pcap_create(iface, errbuf);

    hlt_free(iface);

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
    src->handle = 0;
    return 0;
}

hlt_iosrc* hlt_iosrc_new_offline(hlt_string interface, hlt_exception** excpt,
                                 hlt_execution_context* ctx)
{
    hlt_iosrc* src = GC_NEW(hlt_iosrc, ctx);
    src->type = Hilti_IOSrc_PcapOffline;
    src->iface = hlt_string_copy(interface, excpt, ctx);
    GC_CCTOR(src->iface, hlt_string, ctx);

    char* iface = hlt_string_to_native(interface, excpt, ctx);
    if ( hlt_check_exception(excpt) )
        return 0;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* p = pcap_open_offline(iface, errbuf);

    hlt_free(iface);

    if ( ! p ) {
        _raise_error(src, errbuf, excpt, ctx);
        return 0;
    }

    src->handle = p;
    return src;
}

hlt_packet hlt_iosrc_read_try(hlt_iosrc* src, int8_t keep_link_layer, hlt_exception** excpt,
                              hlt_execution_context* ctx)
{
    hlt_packet result = {0.0, NULL};

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
            _strip_link_layer(src, (const char**)&data, &caplen, pcap_datalink(src->handle), excpt,
                              ctx);
            if ( hlt_check_exception(excpt) )
                return result;
        }

        // We need to copy it to make sure it remains valid.
        hlt_bytes* pkt = hlt_bytes_new_from_data_copy((const int8_t*)data, caplen, excpt, ctx);
        if ( hlt_check_exception(excpt) )
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
    hlt_set_exception(excpt, &hlt_exception_would_block, 0, ctx);
    return result;
}

void hlt_iosrc_close(hlt_iosrc* src, hlt_exception** excpt, hlt_execution_context* ctx)
{
    pcap_close(src->handle);
    src->handle = 0;
}
