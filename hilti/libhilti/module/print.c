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

#include "module.h"

/*
 * Hilti::print(obj, newline = True)
 *
 * Prints a textual representation of an object to stdout.
 *
 * obj: instance of any HILTI type - The object to print.
 * newline: bool - If true, a newline is added automatically.
 *
 */
void hilti_print(const hlt_type_info* type, void* obj, int8_t newline, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // To prevent race conditions with multiple threads, we have to lock stdout here and then
    // unlock it at each possible exit to this function.
    flockfile(stdout);

    if ( type->to_string ) {
        hlt_string s = (*type->to_string)(type, obj, 0, excpt, ctx);
        if ( *excpt )
        {
            funlockfile(stdout);
            return;
        }

        hlt_string_print(stdout, s, 0, excpt, ctx);
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
}

