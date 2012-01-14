///
/// libhilti's debugging output infrastructure.
///

#include <string.h>
#include <stdarg.h>

#include "types.h"
#include "config.h"
#include "string_.h"

static int _want_stream(hlt_string stream, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* dbg = hlt_config_get()->debug_streams;

    if ( ! dbg )
        return 0;

    const char* s = hlt_string_to_native(stream, excpt, ctx);

    // FIXME: It's not very efficient to go through the stream-list every
    // time. Let's see if it's worth optimizing this even for the debug mode.
    char* t;
    char *saveptr;
    char* copy = strdup(dbg);
    char* c = copy;

    while ( (t = strtok_r(c, ":", &saveptr)) ) {
        if ( strcmp(s, t) == 0 )
            return 1;
        c = 0;
    }

    return 0;
}

static void _make_prefix(hlt_string stream, char* dst, int len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* s = hlt_string_to_native(stream, excpt, ctx);

#if 0
    if ( hlt_is_multi_threaded() ) {
        const char* t = hlt_thread_mgr_current_native_thread(__hlt_global_thread_mgr);
        snprintf(dst, len, "[%s/%s] ", s, t);
    }
    else
#endif    
        snprintf(dst, len, "[%s] ", s);
}

void __hlt_debug_init()
{
#ifdef DEBUG
    // This is easy to forget so let's print a reminder.
    // __hlt_debug_printf_internal("hilti-trace", "Reminder: hilti-trace requires compiling with debugging level > 1.");
    // __hlt_debug_printf_internal("hilti-flow",  "Reminder: hilti-flow requires compiling with debugging level > 1.");
#endif
}

void __hlt_debug_done()
{
#ifdef DEBUG
    // Nothing to do right now.
#endif
}

#if 0

static hlt_string_constant INDENT = { 3, "   " };

void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! _want_stream(stream, excpt, ctx) )
        return;

    char prefix[128];

    _make_prefix(stream, prefix, sizeof(prefix), excpt, ctx);

    hlt_string usr = hilti_fmt(fmt, type, tuple, excpt, ctx);

    for ( int i = ctx->debug_indent; i; --i )
        usr = hlt_string_concat(&INDENT, usr, excpt, ctx);

    if ( *excpt )
        return;

    // We must not terminate while in here.
    int old_state;
    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

    FILE* out = hlt_config_get()->debug_out;
    flockfile(out);

    hlt_string_print_n(out, hlt_string_concat(hlt_string_from_asciiz(prefix, excpt, ctx), usr, excpt, ctx), 1, 256, excpt, ctx);

    fflush(out);
    funlockfile(out);

    hlt_pthread_setcancelstate(old_state, NULL);
}

void __hlt_debug_printf_internal(const char* s, const char* fmt, ...)
{
    hlt_exception* excpt = 0;

    hlt_string stream = hlt_string_from_asciiz(s, &excpt, __hlt_global_execution_context);

    if ( ! _want_stream(stream, &excpt, __hlt_global_execution_context) )
        return;

    char buffer[512];
    _make_prefix(stream, buffer, sizeof(buffer), &excpt, __hlt_global_execution_context);

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

    // We must not terminate while in here.
    int old_state;
    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

    FILE* out = hlt_config_get()->debug_out;
    flockfile(out);

    fputs(buffer, out);

    fflush(out);
    funlockfile(out);

    hlt_pthread_setcancelstate(old_state, NULL);
}

void __hlt_debug_print_str(const char* msg, hlt_execution_context* ctx)
{
    fprintf(stderr, "debug: %3" PRId64 " ", ctx->debug_indent);

    for ( int i = ctx->debug_indent * 4; i; --i )
        fputc(' ', stderr);

    fprintf(stderr, "%s\n", msg);
}

void __hlt_debug_print_ptr(const char* s, void* ptr, hlt_execution_context* ctx)
{
    fprintf(stderr, "debug: %3" PRId64 " ", ctx->debug_indent);

    for ( int i = ctx->debug_indent * 4; i; --i )
        fputc(' ', stderr);

    fprintf(stderr, "%s %p\n", s, ptr);
}

void __hlt_debug_push_indent(hlt_execution_context* ctx)
{
    ++ctx->debug_indent;
}

void __hlt_debug_pop_indent(hlt_execution_context* ctx)
{
    if ( ctx->debug_indent > 0 )
        --ctx->debug_indent;
}

#endif