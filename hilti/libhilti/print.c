/* $Id$
 * 
 * printf() function for libhilti.
 * 
 */

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "hilti_intern.h"
#include "utf8proc.h"

// Pthreads structures used for locking.
static pthread_mutex_t __hlt_print_lock = PTHREAD_MUTEX_INITIALIZER;


/* FIXME: This function doesn't print non-ASCII Unicode codepoints as we can't 
 * convert to the locale encoding yet. We just print them in \u syntax. */
static void _print_str(const __hlt_string* s, __hlt_exception* excpt)
{
    if ( ! s )
        // Empty string.
        return;
    
    int32_t cp;
    const int8_t* p = s->bytes;
    const int8_t* e = p + s->len;
    
    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &cp);
        
        if ( n < 0 ) {
            *excpt = __hlt_exception_value_error;
            return;
        }
        
        if ( cp < 128 )
            fputc(cp, stdout);
        else {
            // FIXME: We should bring our own itoa().
            if ( cp < (1 << 16) )
                fprintf(stdout, "\\u%04x", cp);
            else
                fprintf(stdout, "\\U%08x", cp);
        }
        
        p += n;
    }
}

/*
 * Hilti::print(obj, newline = True)
 * 
 * Prints a textual representation of an object to stdout.
 * 
 * obj: instance of any HILTI type - The object to print. 
 * newline: bool - If true, a newline is added automatically.
 * 
 */
void hilti_print(const __hlt_type_info* type, void* obj, int8_t newline, __hlt_exception* excpt)
{
    pthread_mutex_lock(&__hlt_print_lock);

    // To prevent race conditions with multiple threads, we have to lock stdout here and then
    // unlock it at each possible exit to this function.
    flockfile(stdout);

    if ( type->to_string ) {
        __hlt_string *s = (*type->to_string)(type, obj, 0, excpt);
        if ( *excpt )
        {
            funlockfile(stdout);
            return;
        }

        _print_str(s, excpt);
        if ( *excpt )
        {
            funlockfile(stdout);
            return;
        }
    }
    
    else {
        /* No fmt function, just print the tag. */
        fprintf(stdout, "<%s>", type->tag);
    }
    
    if ( newline )
        fputc('\n', stdout);

    fflush(stdout);

    funlockfile(stdout);

    pthread_mutex_unlock(&__hlt_print_lock);
}

