// $Id$

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <pcap.h>

extern "C" {
#include <libhilti.h>
}

const char *Trace;
int Profile = 0;

unsigned long HdrSize = 14; // Ethernet

pcap_t* Pcap = 0;
int Dummy = 0;
unsigned long PacketCounter = 0;

const char* fmt_log(const char* prefix, const char* fmt, va_list ap)
{
    const int SIZE = 32768;
    static char buffer[SIZE];
    int n = 0;

    n += snprintf(buffer + n, SIZE - n, "%s: ", prefix);
    n += vsnprintf(buffer + n, SIZE - n, fmt, ap);

    strcat(buffer + n, "\n");

    return buffer;
}

void error(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char* msg = fmt_log("error", fmt, ap);
    va_end(ap);
    fputs(msg, stdout);
    exit(1);
}

extern "C" {
extern int8_t bpf2hlt_filter(hlt_bytes *, hlt_exception **, hlt_execution_context*);
extern int8_t bpf2hlt_filter_noop(hlt_bytes *, hlt_exception **, hlt_execution_context*);
}

hlt_execution_context* ctx = 0;

void pcapOpen()
{
    static char errbuf[PCAP_ERRBUF_SIZE];

    Pcap = pcap_open_offline(const_cast<char*>(Trace), errbuf);

    if ( ! Pcap )
        error("%s", errbuf);

    int dl = pcap_datalink(Pcap);
    if ( dl != DLT_EN10MB )
        error("unknown data link type 0x%x", dl);
}

inline uint64_t rdtsc()
{
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t cycles = 0;

bool pcapNext()
{
    struct pcap_pkthdr* hdr;
    const u_char* data;
    int result;

    result = pcap_next_ex(Pcap, &hdr, &data);

    if ( result < 0 )
        // error("no more input");
        return false;

    if ( result == 0 )
        return false;

    // Do an extra copy here to compare with HILTI, which does so as well.
    size_t len = hdr->caplen - HdrSize;
    int8_t* buffer = (int8_t*) hlt_malloc_no_init(len);
    memcpy((char *)buffer, data + HdrSize, len);

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_bytes* b = hlt_bytes_new_from_data(buffer, len, &excpt, ctx);

    int8_t match = 0;

    if ( Dummy != 2 )  {
        if ( Dummy ) {
            uint64_t n = rdtsc();
            match = bpf2hlt_filter_noop(b, &excpt, ctx);
            cycles += (rdtsc() - n );
        }
        else {
            uint64_t n = rdtsc();
            match = bpf2hlt_filter(b, &excpt, ctx);
            cycles += (rdtsc() - n );
        }
    }

    GC_DTOR(b, hlt_bytes);

    if ( match )
        ++PacketCounter;

    return true;
}

void pcapClose()
{
    pcap_close(Pcap);
}

void usage()
{
    printf("pktcnt [Options] <input file>\n"
           "\n"
           "  -r| --readfile <trace>         Trace file to read\n"
           "  -P| --profile                  Enable HILTI profiling\n"
           "\n");

    exit(1);
}

static struct option long_options[] = {
    {"readfile", required_argument, 0, 't'},
    {"profile", required_argument, 0, 'P'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv)
{
    hlt_init();

    while (1) {
        char c = getopt_long (argc, argv, "r:Pd", long_options, 0);

        if ( c == -1 )
            break;

        switch ( c ) {
          case 'r':
            Trace = optarg;
            break;

          case 'd':
            ++Dummy;
            break;

          case 'P':
            Profile = 1;
            break;

          default:
            usage();
        }
    }

    if ( optind != argc )
        usage();

    if ( ! Trace )
        error("need -r");

    hlt_exception* excpt = 0;
    hlt_string profiler_tag = hlt_string_from_asciiz("pktcnt-total", &excpt, hlt_global_execution_context());

    if ( Profile ) {
        fprintf(stderr, "Enabling profiling ...\n");
        hlt_config cfg = *hlt_config_get();
        cfg.profiling = 1;
        hlt_config_set(&cfg);
    }

    pcapOpen();

    if ( Profile ) {
        hlt_exception* excpt = 0;
        hlt_profiler_start(profiler_tag, Hilti_ProfileStyle_Standard, 0, 0, &excpt, hlt_global_execution_context());
    }

    PacketCounter = 0;

    ctx = hlt_global_execution_context();

    fprintf(stderr, "pcap loop ...\n");

    while ( pcapNext() );

    if ( Profile ) {
        hlt_exception* excpt = 0;
        hlt_profiler_stop(profiler_tag, &excpt, hlt_global_execution_context());
    }

    pcapClose();

    fprintf(stdout, "packets %lu\n", PacketCounter);

    const char* tag = "hlt";

    if ( Dummy == 2 )
        tag = "hltnofilter";

    if ( Dummy == 1 )
        tag = "hltemptyfilter";

    fprintf(stderr, "%s cycles %lu\n", tag, cycles);

    return 0;
    }



