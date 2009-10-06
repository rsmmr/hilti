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

#include "hilti.h"
#include "utf8proc.h"

/* FIXME: This function doesn't print non-ASCII Unicode codepoints as we can't 
 * convert to the locale encoding yet. We just print them in \u syntax. */
void __hlt_print_str(FILE* file, hlt_string s, int8_t newline, hlt_exception** excpt)
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
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }
        
        if ( cp < 128 )
            fputc(cp, file);
        else {
            // FIXME: We should bring our own itoa().
            if ( cp < (1 << 16) )
                fprintf(file, "\\u%04x", cp);
            else
                fprintf(file, "\\U%08x", cp);
        }
        
        p += n;
    }
    
    if ( newline )
        fputc('\n', file);
        
}
