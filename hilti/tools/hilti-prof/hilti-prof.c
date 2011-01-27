//
// Converts a HILTI profiling file into readable output.

#include <hilti.h>

int optPretty = 1;

static void usage()
{
    fprintf(stderr, "hilti-prof <hlt-prof.dat>\n");
    exit(1);
}

static void printHeader(time_t t)
{
    fputs("#! "
        "ctime "
        "cwall "
        "tag "
        "type "
        "time "
        "wall "
        "updates "
        "cycles "
        "misses "
        "alloced "
        "heap "
        "user "
        "\n",
        stdout
        );
    fputs("#\n", stdout);
    fputs("# ", stdout);
    fputs(ctime(&t), stdout);
    fputs("#\n", stdout);
}

static const char* fmtType(int8_t type)
{
    switch ( type ) {
      case HLT_PROFILER_START:
        return "B";

      case HLT_PROFILER_UPDATE:
        return "U";

      case HLT_PROFILER_SNAPSHOT:
        return "S";

      case HLT_PROFILER_STOP:
        return "E";
    }

    fprintf(stderr, "unknown snapshot type %d\n", type);
    exit(1);
}

static void printRecord(const char* tag, const hlt_profiler_record* rec)
{
    printf("%" PRIu64 " %" PRIu64 " %s %s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
           rec->ctime, rec->cwall, tag, fmtType(rec->type), rec->time, rec->wall, rec->updates, rec->cycles, rec->misses, rec->alloced, rec->heap, rec->user);
}

int main(int argc, char** argv)
{
    if ( argc != 2 )
        usage();

    time_t t;
    int fd = hlt_profiler_file_open(argv[1], &t);

    if ( fd < 0 ) {
        fprintf(stderr, "error opening input file\n");
        return 1;
    }

    char tag[HLT_PROFILER_MAX_TAG_LENGTH];
    hlt_profiler_record rec;

    printHeader(t);

    while ( 1 ) {

        int ret = hlt_profiler_file_read(fd, tag, &rec);

        if ( ret == 0 )
            // Eof.
            break;

        if ( ret < 0 ) {
            perror("cannot read profiler record");
            return 1;
        }

        printRecord(tag, &rec);

    }

    hlt_profiler_file_close(fd);

    return 0;
}

