// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <binpac.h>

#include "pac/test_pac.h"

void usage()
{
    fprintf(stderr, "bptool -a <analyzer> [options]\n\n");
    fprintf(stderr, "  -o <file>     Feed <file> to originator side\n");
    fprintf(stderr, "  -r <file>     Feed <file> to responder side\n");
    fprintf(stderr, "\n  available analyzers: test\n");
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

void feed_file(FILE* f, binpac::ConnectionAnalyzer* conn, bool is_orig)
{
    static const int bufsize = 128;
    u_char buffer[bufsize];
    
    while ( true ) {
        
        int n = fread(buffer, 1, bufsize, f);

        if ( n ) 
            conn->NewData(is_orig, buffer, buffer + n);
        
        if ( n < bufsize )
            break;
        
    }
}

int main(int argc, char** argv)
{
    char c;
    const char *analyzer = 0;
    const char *orig_file = 0;
    const char *resp_file = 0;

    while ((c = getopt (argc, argv, "a:o:r:")) != -1)
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
            
          default:
            usage();
        }
    
    if ( ! analyzer )
        usage();
    
    binpac::ConnectionAnalyzer* conn = 0;
    
    if ( strcmp(analyzer, "test") == 0 )
        conn = new binpac::Test::Test_Conn;
    
    if ( ! conn ) {
        fprintf(stderr, "unknown analyzer '%s'\n", analyzer);
        exit(1);
    }
        
    if ( orig_file )
        feed_file(open_file(orig_file), conn, true);
    
    if ( resp_file )
        feed_file(open_file(resp_file), conn, false);
    
    return 0;
}
     
