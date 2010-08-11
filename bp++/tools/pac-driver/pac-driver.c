// $Id$
// 
// A stand-alone driver for BinPAC++ generated parsers.
 
#include <stdio.h>
#include <stdio.h>
#include <getopt.h>

#include <hilti.h>
#include <binpac.h>

int debug = 0;
int debug_hooks = 0;

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);
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
    fprintf(stderr, "    -B            Enable BinPAC++ debugging hooks\n");
    fprintf(stderr, "    -i  <n>       Feed input incrementally in chunks of size <n>\n");
    fprintf(stderr, "    -v            Enable verbose output\n");
    fprintf(stderr, "\n");

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();
    
    hlt_list* parsers = binpac_parsers(&excpt, ctx);

    if ( hlt_list_size(parsers, &excpt, ctx) == 0 ) {
        fprintf(stderr, "  No parsers defined.\n\n");
        exit(1);
    }
    
    fprintf(stderr, "  Available parsers: \n\n");

    hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_list_iter end = hlt_list_end(parsers, &excpt, ctx);

    int count = 0;
    
    while ( ! (hlt_list_iter_eq(i, end, &excpt, ctx) || excpt) ) {
        ++count;
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);

        fputs("    ", stderr);
        
        hlt_string_print(stderr, p->name, 0, &excpt, ctx);
        
        for ( int n = 14 - hlt_string_len(p->name, &excpt, ctx); n; n-- )
            fputc(' ', stderr);
        
        hlt_string_print(stderr, p->description, 1, &excpt, ctx);
        
        i = hlt_list_iter_incr(i, &excpt, ctx);
    }

    check_exception(excpt);
    
    if ( ! count )
        fprintf(stderr, "    None.\n");
    
    fputs("\n", stderr);
    
    exit(1);
}

static binpac_parser* findParser(const char* name)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    
    hlt_list* parsers = binpac_parsers(&excpt, ctx);
    
    hlt_string hname = hlt_string_from_asciiz(name, &excpt, ctx);
    
    hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_list_iter end = hlt_list_end(parsers, &excpt, ctx);

    while ( ! (hlt_list_iter_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);
        
        if ( hlt_string_cmp(hname, p->name, &excpt, ctx) == 0 )
            return p;
        
        i = hlt_list_iter_incr(i, &excpt, ctx);
    }
    
    check_exception(excpt);
    return 0;
}

hlt_bytes* readInput()
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    hlt_bytes* input = hlt_bytes_new(&excpt, ctx);
    check_exception(excpt);
    
    int8_t buffer[256];
    
    while ( ! excpt ) {
        size_t n = fread(buffer, 1, sizeof(buffer), stdin);
        
        int8_t* copy = hlt_gc_malloc_atomic(n);
        memcpy(copy, buffer, n);
        hlt_bytes_append_raw(input, copy, n, &excpt, ctx);
        
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

void parseInput(binpac_parser* p, hlt_bytes* input, int chunk_size)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    hlt_bytes_pos cur = hlt_bytes_begin(input, &excpt, ctx);
    
    check_exception(excpt);

    if ( ! chunk_size ) {
        // Feed all input at once.
        hlt_bytes_freeze(input, 1, &excpt, ctx);
        (*p->parse_func)(cur, 0, &excpt, ctx);
        check_exception(excpt);
        return;
    }

    // Feed incrementally.
    
    hlt_bytes* chunk;
    hlt_bytes_pos end = hlt_bytes_end(input, &excpt, ctx);
    hlt_bytes_pos cur_end;
    int8_t done = 0;
    hlt_exception* resume = 0;
    
    input = hlt_bytes_new(&excpt, ctx);
    
    while ( ! done ) {
        cur_end = hlt_bytes_pos_incr_by(cur, chunk_size, &excpt, ctx);
        done = hlt_bytes_pos_eq(cur_end, end, &excpt, ctx);
        
        chunk = hlt_bytes_sub(cur, cur_end, &excpt, ctx);
        hlt_bytes_append(input, chunk, &excpt, ctx);
        
        if ( done )
            hlt_bytes_freeze(input, 1, &excpt, ctx);

        check_exception(excpt);
        
        if ( ! resume ) {
            if ( debug )
                fprintf(stderr, "--- pac-driver: starting parsing.\n");
            
            hlt_bytes_pos s = hlt_bytes_begin(input, &excpt, ctx);
            check_exception(excpt);
            
            (*p->parse_func)(s, 0, &excpt, ctx);
        }
        
        else {
            if ( debug )
                fprintf(stderr, "--- pac-driver: resuming parsing.\n");
            
            (*p->resume_func)(resume, &excpt, ctx);
        }

        if ( excpt ) {
            if ( excpt->type == &hlt_exception_yield ) {
                if ( debug )
                    fprintf(stderr, "--- pac-driver: parsing yielded.\n");
                
                resume = excpt;
                excpt = 0;
            }
            
            else
                check_exception(excpt);
        }
        
        else if ( ! done ) {
            if ( debug )
                fprintf(stderr, "pac-driver: end of input reached even though more could be parsed.");
            
            break;
        }
        
        cur = cur_end;
    }
}

int main(int argc, char** argv)
{
    int verbose = 0;
    int chunk_size = 0;
    const char* parser = 0;

    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    
    char ch;
    while ((ch = getopt(argc, argv, "i:p:vdBh")) != -1) {
        
        switch (ch) {
            
          case 'i':
            chunk_size = atoi(optarg);
            break;
            
          case 'p':
            parser = optarg;
            break;
            
          case 'v':
            verbose = 1;
            break;
            
          case 'd':
            ++debug;
            break;
            
          case 'B':
            debug_hooks = 1;
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
    
    binpac_enable_debugging(debug_hooks);
    
    binpac_parser* p = 0;
    
    if ( ! parser ) {
        hlt_exception* excpt = 0;
        hlt_list* parsers = binpac_parsers(&excpt, ctx);
    
        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt, ctx) == 1 ) {
            hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
            p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);
            assert(p);
        }
        
        else { 
            // If we don't have any parsers, we do nothing and just exit
            // normally.
            if ( hlt_list_size(parsers, &excpt, ctx) == 0 )
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

    parseInput(p, input, chunk_size);
    
    exit(0);
    
}
