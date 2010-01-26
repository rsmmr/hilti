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
        hlt_exception_print_uncaught(excpt);
        exit(1);
    }
}

static void usage(const char* prog) 
{
    fprintf(stderr, "%s [options]\n\n", prog);
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -p <parser>   Use given parser\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -d            Enable basic BinPAC++ debug output\n");
    fprintf(stderr, "    -dd           Enable detailed BinPAC++ debug output\n");
    fprintf(stderr, "    -v            Enable verbose output\n");
    fprintf(stderr, "\n");

    hlt_exception* excpt = 0;
    hlt_list* parsers = binpac_parsers(&excpt);

    if ( hlt_list_size(parsers, &excpt) == 0 ) {
        fprintf(stderr, "  No parsers defined.\n\n");
        exit(1);
    }
    
    fprintf(stderr, "  Available parsers: \n\n");

    hlt_list_iter i = hlt_list_begin(parsers, &excpt);
    hlt_list_iter end = hlt_list_end(parsers, &excpt);

    int count = 0;
    
    while ( ! (hlt_list_iter_eq(i, end, &excpt) || excpt) ) {
        ++count;
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt);

        fputs("    ", stderr);
        
        hlt_string_print(stderr, p->name, 0, &excpt);
        
        for ( int n = 14 - hlt_string_len(p->name, &excpt); n; n-- )
            fputc(' ', stderr);
        
        hlt_string_print(stderr, p->description, 1, &excpt);
        
        i = hlt_list_iter_incr(i, &excpt);
    }

    check_exception(excpt);
    
    if ( ! count )
        fprintf(stderr, "    None.\n");
    
    fputs("\n", stderr);
    
    exit(1);
}

static binpac_parser* findParser(const char* name)
{
    hlt_exception* excpt = 0;
    
    hlt_list* parsers = binpac_parsers(&excpt);
    
    hlt_string hname = hlt_string_from_asciiz(name, &excpt);
    
    hlt_list_iter i = hlt_list_begin(parsers, &excpt);
    hlt_list_iter end = hlt_list_end(parsers, &excpt);

    while ( ! (hlt_list_iter_eq(i, end, &excpt) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt);
        
        if ( hlt_string_cmp(hname, p->name, &excpt) == 0 )
            return p;
        
        i = hlt_list_iter_incr(i, &excpt);
    }
    
    check_exception(excpt);
    return 0;
}

hlt_bytes* readInput()
{
    hlt_exception* excpt = 0;
    hlt_bytes* input = hlt_bytes_new(&excpt);
    check_exception(excpt);
    
    int8_t buffer[256];
    
    while ( ! excpt ) {
        size_t n = fread(buffer, 1, sizeof(buffer), stdin);
        
        int8_t* copy = hlt_gc_malloc_atomic(n);
        memcpy(copy, buffer, n);
        hlt_bytes_append_raw(input, copy, n, &excpt);
        
        if ( feof(stdin) )
            break;
        
        if ( ferror(stdin) ) {
            fprintf(stderr, "error while reading from stdin\n");
            exit(1);
        }
    }
     
    check_exception(excpt);
    
    return input;
        
}

void parseInput(binpac_parser* p, hlt_bytes* input)
{
    hlt_exception* excpt = 0;
    hlt_bytes_pos begin = hlt_bytes_begin(input, &excpt);
    check_exception(excpt);
    
    (*p->parse_func)(begin, &excpt);
    check_exception(excpt);
}

int main(int argc, char** argv)
{
    int debug = 0;
    int verbose = 0;
    const char* parser = 0;

    hilti_init();
    
    char ch;
    while ((ch = getopt(argc, argv, "p:vdh")) != -1) {
        
        switch (ch) {
            
          case 'p':
            parser = optarg;
            
          case 'v':
            verbose = 1;
            break;
            
          case 'd':
            ++debug;
            break;
            
          case 'h':
            // Fall-through.
          default:
            usage(argv[0]);
        }
    }
        
    argc -= optind;
    argv += optind;
    
    if ( argc != 0 )
        usage(argv[0]);

    binpac_parser* p = 0;
    
    if ( ! parser ) {
        hlt_exception* excpt = 0;
        hlt_list* parsers = binpac_parsers(&excpt);
    
        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt) == 1 ) {
            hlt_list_iter i = hlt_list_begin(parsers, &excpt);
            p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt);
            assert(p);
        }
        
        else { 
            // If we don't have any parsers, we do nothing and just exit
            // normally.
            if ( hlt_list_size(parsers, &excpt) == 0 )
                exit(0);
        
            fprintf(stderr, "no parser specified; see usage for list.\n");
            exit(1);
            }
    }
 
    else {
        p = findParser(parser);
        if ( ! p ) {
            fprintf(stderr, "unknown parser '%s'. See usage for list.\n", parser);
            exit(1);
            }
    }

    assert(p);

    hlt_bytes* input = readInput();

    parseInput(p, input);
    
    exit(0);
    
}
