
#include <ctype.h>

#include <libhilti.h>

#include "acl.tmp.h"

void read_packets()
{
    static char buffer[128];
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    int count_yes = 0;
    int count_no = 0;

    while( fgets(buffer, sizeof(buffer), stdin) ) {

        if ( buffer[0] == '!' )
            continue;

        buffer[strlen(buffer) - 1] = '\0';

        char* t = buffer;

        char* addr1 = strchr(t, ' ');
        if ( ! addr1 )
            continue;

        *addr1++ = '\0';

        while( *addr1 && isspace(*addr1) )
            ++addr1;

        char* addr2 = strchr(addr1, ' ');
        if ( ! addr2 )
            continue;

        *addr2++ = '\0';

        while( *addr2 && isspace(*addr2) )
            ++addr2;

        if ( strcmp(addr1, "-") == 0 || strcmp(addr2, "-") == 0 )
            continue;

/*
        char* flags = strchr(addr2, ' ');
        if ( ! flags )
            flags = addr2 + strlen(addr2);

        *flags++ = '\0';

        int8_t syn = (strchr(flags, 'S') != 0);
*/

        double ts = strtod(t, 0);
        hlt_time ht = hlt_time_from_timestamp(ts);
        hlt_addr ha1 = hlt_addr_from_asciiz(addr1, &excpt, ctx);
        hlt_addr ha2 = hlt_addr_from_asciiz(addr2, &excpt, ctx);

        int8_t m = acl2hlt_match_packet(ht, ha1, ha2, &excpt, ctx);

        if ( m == 1)
            count_yes++;
        else if ( m == 0 )
            count_no++;

        // fprintf(stderr, "|%s|%s|%s| -> %d\n", t, addr1, addr2, m);
    }

    fprintf(stdout, "yes: %d\n", count_yes);
    fprintf(stdout, "no : %d\n", count_no);
}

int main(int argc, const char** argv)
{
    if ( argc > 1 ) {
        fprintf(stderr, "usage: ipsumdump -t -s -d | %s\n", argv[0]);
        exit(1);
    }

    if ( argc == 2 )
        stdin = fopen(argv[1], "r");

    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    acl2hlt_init_classifier(&excpt, ctx);

    if ( excpt ) {
        fprintf(stderr, "cannot init classifier\n");
        hlt_exception_print(excpt, ctx);
        exit(1);
    }

    read_packets();
}

