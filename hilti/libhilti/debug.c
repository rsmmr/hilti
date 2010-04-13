// $Id$

#include <string.h>
#include <stdarg.h>

#include "hilti.h"
#include "list.h"

extern const hlt_type_info hlt_type_info_string;
extern const hlt_type_info hlt_type_info_tuple_string;

static int _want_stream(hlt_string stream, hlt_exception** excpt)
{
    // FIXME: It's not very efficient to go through the stream-list every
    // time. Let's see if it's worth optimizing this even for the debug mode.
    // Once we have hash tables, this should be easy to do.
    const hlt_config* cfg = hlt_config_get();
    hlt_list* streams = cfg->debug_streams;
    
    if ( ! streams )
        return 0;
    
    hlt_list_iter i = hlt_list_begin(streams, excpt);
    hlt_list_iter end = hlt_list_end(streams, excpt);
    
    int found = 0;
    while ( ! hlt_list_iter_eq(i, end, excpt) ) {
        hlt_string* s = hlt_list_iter_deref(i, excpt);
        if ( hlt_string_cmp(stream, *s, excpt) == 0) {
            found = 1;
            break;
        }
        
        i = hlt_list_iter_incr(i, excpt);
    }

    if ( *excpt )
        return 0;

    return found;
}

static void _make_prefix(hlt_string stream, char* dst, int len, hlt_exception** excpt)
{
    const char* s = hlt_string_to_native(stream, excpt);
    
    if ( hlt_is_multi_threaded() ) {
        const char* t = hlt_thread_mgr_current_native_thread(__hlt_global_thread_mgr);
        snprintf(dst, len, "[%s/%s] ", s, t);
    }
    else
        snprintf(dst, len, "[%s] ", s);
}

void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt)
{
    if ( ! _want_stream(stream, excpt) )
        return;

    char prefix[128];
    _make_prefix(stream, prefix, sizeof(prefix), excpt);
    
    hlt_string usr = hilti_fmt(fmt, type, tuple, excpt);

    if ( *excpt )
        return;

    FILE* out = hlt_config_get()->debug_out;
    flockfile(out);
    
    hlt_string_print(out, hlt_string_concat(hlt_string_from_asciiz(prefix, excpt), usr, excpt), 1, excpt);
    
    fflush(out);
    funlockfile(out);
}

void __hlt_debug_printf_internal(const char* s, const char* fmt, ...)
{
    hlt_exception* excpt = 0;
    
    hlt_string stream = hlt_string_from_asciiz(s, &excpt);
    
    if ( ! _want_stream(stream, &excpt) )
        return;

    char buffer[512];
    _make_prefix(stream, buffer, sizeof(buffer), &excpt);

    if ( excpt ) {
        fprintf(stderr, "exception in __hlt_debug_printf_internal\n");
        return;
    }

    int len = strlen(buffer);
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, sizeof(buffer) - len, fmt, args);
    va_end(args);
    
    strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
    
    FILE* out = hlt_config_get()->debug_out;
    flockfile(out);
    
    fputs(buffer, out);
    
    fflush(out);
    funlockfile(out);
}


hlt_list* hlt_debug_parse_streams(const char* streams, hlt_exception** excpt)
{
    hlt_list* l = hlt_list_new(&hlt_type_info_string, excpt);
    if ( *excpt )
        return 0;
    
    char* t;
    char* c;
    char *saveptr;
    
    char* copy = c = hlt_gc_malloc_atomic(strlen(streams) + 1);
    strcpy(copy, streams);

    while ( (t = strtok_r(c, ":", &saveptr)) ) {
        hlt_string s = hlt_string_from_asciiz(t, excpt);
        hlt_list_push_back(l, &hlt_type_info_string, &s, excpt);
        if ( *excpt ) 
            return 0;
        
        c = 0;
    }
    
    return l;
}

static int indent = 0;

void __hlt_debug_print_str(const char* msg)
{
    fprintf(stderr, "debug: %3d ", indent);
   
    for ( int i = indent * 4; i; --i )
        fputc(' ', stderr);
    
    fprintf(stderr, "%s\n", msg);
}

void __hlt_debug_print_ptr(const char* s, void* ptr)
{
    fprintf(stderr, "debug: %3d ", indent);
    
    for ( int i = indent * 4; i; --i )
        fputc(' ', stderr);
    
    fprintf(stderr, "%s %p\n", s, ptr);
}

void __hlt_debug_push_indent()
{
    ++indent;
}
void __hlt_debug_pop_indent()
{
    --indent;
}

