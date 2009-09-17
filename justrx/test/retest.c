// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jrx.h"
#include "regex.h"

static void print_error(int rc, regex_t* re, const char* prefix)
{
    char buffer[128];
    regerror(rc, re, buffer, sizeof(buffer));
    printf("%s, %s\n", prefix, buffer);
}

static void do_match(const char* pattern, const char* buffer, jrx_option options)
{
    const int max_captures = 20;
    
    regex_t re;
    regmatch_t pmatch[max_captures];

    int rc = regcomp(&re, pattern, REG_EXTENDED | options);

    if ( rc != 0 ) {
        print_error(rc, &re, "compile error");
        return;
    }
        
    rc = regexec(&re, buffer, max_captures, pmatch, 0);
    
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
    
    jrx_option debug = 0;
    
    if ( argc > opt && strcmp(argv[opt], "-d") == 0 ) {
        debug = REG_DEBUG;
        ++opt;
    }

    if ( (argc - opt) != 2 ) {
        fprintf(stderr, "usage: retest [-d] <pattern> <string>\n");
        return 1;
    }

    const char* pattern = argv[opt];
    const char* data = argv[opt+1];
    
    fprintf(stderr, "=== Pattern: %s\n", pattern);
    fprintf(stderr, "=== Data   : %s\n", data); 
    
    fprintf(stderr, "\n=== Standard matcher with subgroups\n");
    do_match(pattern, data, debug);
    
    fprintf(stderr, "\n=== Standard matcher without subgroups\n");
    do_match(pattern, data, debug | REG_NOSUB | REG_STD_MATCHER);
    
    fprintf(stderr, "\n=== Minimal matcher\n");
    do_match(pattern, data, debug | REG_NOSUB);
    
    exit(0);
}
