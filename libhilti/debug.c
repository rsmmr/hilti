///
/// libhilti's debugging output infrastructure.
///

#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include "types.h"
#include "config.h"
#include "string_.h"
#include "module/module.h"

_Atomic(uint_fast64_t) _counter; // = 0 produces error on Apple?

static FILE* _debug_out()
{
    static FILE* debug_out = 0;

    if ( debug_out )
        return debug_out;

    /// TODO: Should use mutex here.

    debug_out = fopen(hlt_config_get()->debug_out, "w");

    if ( ! debug_out ) {
        fprintf(stderr, "hilti: cannot open %s for writing. Aborting.\n", hlt_config_get()->debug_out);
        exit(1);
        }

    setvbuf(debug_out, 0, _IOLBF, 0);

    return debug_out;
}

static int _want_stream(const char* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* dbg = hlt_config_get()->debug_streams;

    if ( ! dbg )
        return 0;

    // FIXME: It's not very efficient to go through the stream-list every
    // time. Let's see if it's worth optimizing this even for the debug mode.

    const char* end = dbg + strlen(dbg);
    const char* j;
    for ( const char* i = dbg; i < end; i = j + 1 ) {
        j = i;

        while ( *j && *j != ':' )
            ++j;

        if ( strlen(s) == j - i && strncmp(s, i, j - i) == 0 )
            return 1;
    }

    return 0;
}

static void _make_prefix(const char* s, char* dst, int len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( hlt_is_multi_threaded() ) {
        const char* t = hlt_thread_mgr_current_native_thread();
        snprintf(dst, len, "%08" PRId64 " [%s/%s] ", ++_counter, s, t);
    }
    else
        snprintf(dst, len, "%08" PRId64 " [%s/-] ", ++_counter, s);

    int min = (len - 1 < 40 ? len - 1 : 40);

    while ( strlen(dst) < min )
        strcat(dst, " ");
}

void __hlt_debug_init()
{
#ifdef DEBUG
    // This is easy to forget so let's print a reminder.
     __hlt_debug_printf_internal("hilti-trace", "Reminder: hilti-trace requires compiling with debugging level > 1.");
     __hlt_debug_printf_internal("hilti-flow",  "Reminder: hilti-flow requires compiling with debugging level > 1.");
#endif
}

void __hlt_debug_done()
{
#ifdef DEBUG
    // Nothing to do right now.
#endif
}

void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
{
    char* s = hlt_string_to_native(stream, excpt, ctx);

    if ( ! _want_stream(s, excpt, ctx) ) {
        hlt_free(s);
        return;
    }

    char prefix[128];

    _make_prefix(s, prefix, sizeof(prefix), excpt, ctx);

    hlt_string usr = hilti_fmt(fmt, type, tuple, excpt, ctx);
    hlt_string indent = hlt_string_from_asciiz("  ", excpt, ctx);
    GC_CCTOR(indent, hlt_string);

    for ( int i = ctx->debug_indent; i; --i ) {
        GC_CCTOR(indent, hlt_string);
        usr = hlt_string_concat_and_unref(indent, usr, excpt, ctx);
    }

    GC_DTOR(indent, hlt_string);

    if ( *excpt )
        return;

    // We must not terminate while in here.
    int old_state;
    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

    FILE* out = _debug_out();
    flockfile(out);

    hlt_string p = hlt_string_from_asciiz(prefix, excpt, ctx);
    hlt_string r = hlt_string_concat_and_unref(p, usr, excpt, ctx);
    hlt_string_print_n(out, r, 1, 256, excpt, ctx);
    GC_DTOR(r, hlt_string);

    fflush(out);
    funlockfile(out);

    hlt_pthread_setcancelstate(old_state, NULL);
}

void __hlt_debug_print(const char* stream, const char* msg)
{
    __hlt_debug_printf_internal(stream, "%s", msg);
}

void __hlt_debug_printf_internal(const char* s, const char* fmt, ...)
{
    hlt_exception* excpt = 0;

    if ( ! _want_stream(s, &excpt, 0) )
        return;

    char buffer[512];
    _make_prefix(s, buffer, sizeof(buffer), &excpt, 0);

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

    FILE* out = _debug_out();
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

