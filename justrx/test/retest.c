// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

static void print_error(int rc, regex_t* re, const char* prefix)
{
    char buffer[128];
    regerror(rc, re, buffer, sizeof(buffer));
    printf("%s, %s\n", prefix, buffer);
}

static void do_match(char** argv, int argc, int opt, int options)
{
    const int max_captures = 20;
    
    int rc;
    regex_t re;
    regmatch_t pmatch[max_captures];

    if ( (argc - opt) == 2 )
        rc = regcomp(&re, argv[opt], REG_EXTENDED | options);
    else {
        jrx_regset_init(&re, -1, REG_EXTENDED | options);
        for ( int i = opt; i < argc - 1; i++ ) {
            rc = jrx_regset_add(&re, argv[i], strlen(argv[i]));
            if ( rc != 0 )
                break;
        }
        
        rc = jrx_regset_finalize(&re);
    }
    
    if ( rc != 0 ) {
        print_error(rc, &re, "compile error");
        return;
    }
        
    rc = regexec(&re, argv[argc-1], max_captures, pmatch, 0);
    
    if ( rc != 0 ) {
        print_error(rc, &re, "pattern not found");
        return;
    }
    
    printf("match found!\n");
    
    for ( int i = 0; i < max_captures; i++ ) {
        if ( pmatch[i].rm_so != -1 )
            printf("  capture group #%d: (%d,%d)\n", i, pmatch[i].rm_so, pmatch[i].rm_eo);
    }
}
    

int main(int argc, char**argv)
{
    int opt = 1;
    
    int debug = 0;
    
    if ( argc > opt && strcmp(argv[opt], "-d") == 0 ) {
        debug = REG_DEBUG;
        ++opt;
    }

    if ( (argc - opt) < 2 ) {
        fprintf(stderr, "usage: retest [-d] <patterns> <string>\n");
        return 1;
    }

    fprintf(stderr, "=== Pattern: %s\n", argv[opt]);
    
    for ( int i = opt + 1; i < argc - 1; i++ )
        fprintf(stderr, "             %s\n", argv[i]);
    
    fprintf(stderr, "=== Data   : %s\n", argv[argc-1]); 
    
    fprintf(stderr, "\n=== Standard matcher with subgroups\n");
    do_match(argv, argc, opt, debug);
 
    exit(1); // FIXME
    
    fprintf(stderr, "\n=== Standard matcher without subgroups\n");
    do_match(argv, argc, opt, debug | REG_NOSUB | REG_STD_MATCHER);
    
    fprintf(stderr, "\n=== Minimal matcher\n");
    do_match(argv, argc, opt, debug | REG_NOSUB);
    
    exit(0);
}
