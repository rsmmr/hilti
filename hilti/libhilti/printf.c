/* $Id$
 * 
 * printf() function for libhilti.
 * 
 */

#include <stdio.h>

#include "hilti_intern.h"
#include "utf8proc.h"

void hilti_print(int32_t n)
{
    printf("%d\n", n);
}

void hilti_print_str(const struct __hlt_string* s)
{
    int32_t cp;
    int8_t* p = s->bytes;
    int8_t* e = p + s->len;
    
    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &cp);
        
        if ( n < 0 ) {
            // __hlt_exception_raise(__hlt_exception_value_error);
            return;
        }
        
        if ( cp < 128 )
            putchar((char)cp);
        else if ( cp < (1 << 16) )
            printf("\\u%04x", cp);
        else 
            printf("\\U%08x", cp);
        
        p += n;
    }
    
    putchar('\n');
}
