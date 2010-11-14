// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include <binpac.h>

#include "pac/test_pac.h"
#include "pac/megad_pac.h"
#include "pac/http_pac.h"

int bptool_verbose = 0;
static int debug = 0;

void usage()
{
    fprintf(stderr, "bptool -a <analyzer> [options]\n\n");
    fprintf(stderr, "  -o <file>     Feed <file> to originator side\n");
    fprintf(stderr, "  -r <file>     Feed <file> to responder side\n");
    fprintf(stderr, "  -U            Read pac-contents-formatted bulk input from stdin\n");
    fprintf(stderr, "  -v            Switch analyzer in verbose output mode (where supported)\n");
    fprintf(stderr, "  -d            Enable driver debugging output\n");
    fprintf(stderr, "\n  available analyzers: test,http\n");
    fprintf(stderr, "\n");
    exit(1);
}

FILE* open_file(const char* name)
{
    FILE *f = fopen(name, "r");
    if ( ! f ) {
        fprintf(stderr, "cannot open %s\n", name);
        exit(1);
    }

    return f;
}

binpac::ConnectionAnalyzer* make_new_conn(const char* analyzer)
{
    binpac::ConnectionAnalyzer* conn = 0;

    if ( strcmp(analyzer, "test") == 0 )
        conn = new binpac::Test::Test_Conn;

#if 0
    if ( strcmp(analyzer, "megad") == 0 )
        conn = new binpac::MegaD::MegaD_Conn;
#endif

    if ( strcmp(analyzer, "http") == 0 )
        conn = new binpac::HTTP::HTTP_Conn;

    if ( ! conn ) {
        fprintf(stderr, "unknown analyzer '%s'\n", analyzer);
        exit(1);
    }

    return conn;
}

void feed_file(FILE* f, const char* analyzer, bool is_orig)
{
    static const int bufsize = 128;
    u_char buffer[bufsize];

    binpac::ConnectionAnalyzer* conn = make_new_conn(analyzer);

    while ( true ) {

        int n = fread(buffer, 1, bufsize, f);

        if ( n )
            conn->NewData(is_orig, buffer, buffer + n);

        if ( n < bufsize )
            break;

    }
}

#include "khash.h"

typedef struct {
    binpac::ConnectionAnalyzer* conn;
    int stopped;
} Flow;

KHASH_MAP_INIT_STR(Flow, Flow*)

static void input_error(const char* line)
{
    fprintf(stderr, "error reading input header: not in expected format.\n");
    exit(1);
}

Flow* bulk_feed_piece(const char* analyzer, Flow* flow, char dir, int eof, const char* data, int size, const char* fid, const char* t)
{
    // Make a copy of the data.
    u_char* tmp = (u_char*)malloc(size);
    assert(tmp);
    memcpy(tmp, data, size);

    Flow* result = 0;

    if ( ! flow ) {
        flow = (Flow*)malloc(sizeof(Flow));
        flow->conn = make_new_conn(analyzer);
        flow->stopped = 0;
        result = flow;
    }

    else {
        if ( 0 ) { // Can't really test for error right now. :-(
            // Past parsing proceeded all the way through and we don't expect
            // further input.
            fprintf(stderr, "%s: error, no further input expected\n", fid);
            flow->stopped = 1;
            return 0;
        }
    }

    if ( flow->stopped )
        // Error encountered earlier, just ignore further input.
        return 0;

    if ( size ) {
        assert(flow->conn);
        flow->conn->NewData(dir == '>', tmp, tmp + size);
    }

    free(tmp);

    if ( 0 ) { // Can't really test for error right now. :-(
        fprintf(stderr, "%s error", fid);
        flow->stopped = 1;
        return 0;
    }

    return result;
}

void feed_bulk(const char* analyzer)
{
    static char buffer[65536];
    static char fid[256];
    static char ts[256];

    khash_t(Flow) *hash = kh_init(Flow);

    while ( true ) {
        char* p = fgets(buffer, sizeof(buffer), stdin);
        char* e = p + strlen(buffer);

        if ( feof(stdin) )
            return;

        if ( ! p ) {
            fprintf(stderr, "error reading input header: %s\n", strerror(errno));
            exit(1);
        }

        if ( e > p )
            *(e-1) = '\0'; // Kill the NL.

        if ( p[0] != '#' )
            input_error(buffer);

        // Format is
        //
        //    # <kind> <dir> <len> <fid> <time>

        char kind = p[2];
        char dir = p[4];

        int size = atoi(&p[6]);

        char* f = &p[6];
        while ( *f++ != ' ' );

        char* t = f;
        while ( *t++ != ' ' );
        *(t-1) = '\0';

        if ( t > e )
            input_error(buffer);

        strncpy(fid, f, sizeof(fid) - 3);
        fid[sizeof(fid)-1] = '0';

        strncpy(ts, t, sizeof(ts) - 3);
        ts[sizeof(ts)-1] = '0';

        // Make the flow ID uni-directional and look it up in the state table.
        int len = strlen(fid);
        fid[len] = '#';
        fid[len+1] = dir;
        fid[len+2] = '\0';
        khiter_t i = kh_get(Flow, hash, fid);

        int ignore = 0;
        int eof = 0;
        int known_flow = (i != kh_end(hash));

        if ( debug )
            fprintf(stderr, "--- pac-driver: kind=%c dir=%c size=%d fid=|%s| t=|%s| known=%d\n", kind, dir, size, fid, ts, known_flow);

        if ( kind == 'G' ) {
            // Can't handle gaps yet. Mark flow as to be ignored by setting parser to null.
            kh_value(hash, i) = 0;
            continue;
        }

        if ( known_flow && ! kh_value(hash, i) )
            ignore = 1;

        if ( kind == 'T' ) {
            size = 0;
            eof = 1;
        }

        if ( kind == 'D' ) {
            if ( size > sizeof(buffer) ) {
                fprintf(stderr, "error reading input: data chunk unexpected large (%d)\n", size);
                exit(1);
            }

            if ( fread(buffer, size, 1, stdin) != 1 ) {
                fprintf(stderr, "error reading input chunk: %s\n", strerror(errno));
                exit(1);
            }
        }

        if ( ! known_flow ) {
            int ret;
            i = kh_put(Flow, hash, strdup(fid), &ret);
            kh_value(hash, i) = 0;
        }

        if ( ! ignore ) {
            Flow* result = bulk_feed_piece(analyzer, kh_value(hash, i), dir, eof, buffer, size, fid, ts);

            if ( result )
                kh_value(hash, i) = result;
        }

        if ( eof && kh_value(hash, i) ) {
            delete kh_value(hash, i)->conn;
            free(kh_value(hash, i));
            kh_del(Flow, hash, i);
        }
    }
}

int main(int argc, char** argv)
{
    char c;
    int bulk = 0;

    const char *analyzer = 0;
    const char *orig_file = 0;
    const char *resp_file = 0;

    while ((c = getopt (argc, argv, "a:o:r:vUd")) != -1)
        switch (c) {
          case 'a':
            analyzer = optarg;
            break;

          case 'o':
            orig_file = optarg;
            break;

          case 'r':
            resp_file = optarg;
            break;

          case 'U':
            bulk = 1;
            break;

          case 'v':
            bptool_verbose = 1;
            break;

          case 'd':
            debug = 1;
            break;

          default:
            usage();
        }

    if ( ! analyzer )
        usage();

    if ( orig_file )
        feed_file(open_file(orig_file), analyzer, true);

    if ( resp_file )
        feed_file(open_file(resp_file), analyzer, false);

    if ( bulk )
        feed_bulk(analyzer);

    return 0;
}

