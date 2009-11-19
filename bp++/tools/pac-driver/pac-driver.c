// $Id$
// 
// A stand-alone driver for BinPAC++ generated parsers.
 
#include <stdio.h>
#include <stdio.h>
#include <getopt.h>

#include <hilti.h>
#include <binpac.h>

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hilti_exception_print_uncaught(excpt);
        exit(1);
    }
}

static void usage() 
{
    fprintf(stderr, "pac [options]\n\n");
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -d            Enable basic BinPAC++ debug output\n");
    fprintf(stderr, "    -dd           Enable detailed BinPAC++ debug output\n");
    fprintf(stderr, "    -p <parser>   Use given parser (mandatory)\n");
    fprintf(stderr, "    -v            Enable verbose output\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Available parsers: \n");
    
    hlt_exception* excpt = 0;
    
    hlt_list* parsers = binpac_parsers(&excpt);
    
    hlt_list_iter i = hlt_list_begin(parsers, &excpt);
    hlt_list_iter end = hlt_list_end(parsers, &excpt);

    check_exception(excpt);
    
    while ( ! hlt_list_eq(i, end, &excpt) ) {
        binpac_parser* p = (binpac_parser*) hlt_list_iter_deref(i, &excpt);
        
        fprintf(stderr, "    ");
        hlt_string_print(stderr, p->name, 0, &excpt);
        
        for ( int n = 10 - hlt_string_len(p->name, &excpt); n; n-- )
            fputc(' ', stderr);
        
        hlt_string_print(stderr, p->description, 0, &excpt);
    }
    
    check_exception(excpt);
    
    exit(1);
}

static binpac_parser* getParser(const char* name)
{
    hlt_exception* excpt = 0;
    
    hlt_list* parsers = binpac_parsers(&excpt);
    
    hlt_string hname = hlt_string_from_asciiz(name, &excpt);
    
    hlt_list_iter i = hlt_list_begin(parsers, &excpt);
    hlt_list_iter end = hlt_list_end(parsers, &excpt);

    check_exception(excpt);
    
    while ( ! hlt_list_eq(i, end, &excpt) ) {
        binpac_parser* p = (binpac_parser*) hlt_list_iter_deref(i, &excpt);
        
        if ( hlt_string_cmp(hname, p->name, &excpt) == 0 )
            return p;
    }
    
    check_exception(excpt);
    return 0;
}

int main(int argc, char** argv)
{
    int debug = 0;
    int verbose = 0;
    const char* parser = 0;
    
    char ch;
    while ((ch = getopt(argc, argv, "a:vd")) != -1) {
        
        switch (ch) {
            
          case 'a':
            parser = optarg;
            
          case 'v':
            verbose = 1;
            break;
            
          case 'd':
            ++debug;
            break;
            
          default:
            usage();
        }
    }
        
    if ( argc != 0 )
        usage();
    
    if ( ! parser ) {
        fprintf(stderr, "An parser must be given via -p. See usage for list.\n");
        exit(1);
    }
     
    binpac_parser* p = getParser(parser);
    if ( ! p ) {
        fprintf(stderr, "Unknown parser %s. See usage for list.\n", parser);
        exit(1);
    }

    
    exit(0);
    
}
