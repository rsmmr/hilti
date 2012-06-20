/* $Id$
 *
 * Exception handling functions.
 *
 * Todo: Organize exceptions in a hierarchy.
 *
 */

#include "exceptions.h"
#include "string_.h"
#include "rtti.h"
#include "util.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>

// The mother of all exceptions.
hlt_exception_type hlt_exception_unspecified = { "Unspecified", 0, 0 };

hlt_exception_type hlt_exception_division_by_zero = { "DivisionByZero", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_value_error = { "ValueError", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_out_of_memory = { "OutOfMemory", 0, 0 };
hlt_exception_type hlt_exception_wrong_arguments = { "WrongArguments", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_undefined_value = { "UndefinedValue", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_would_block = { "WouldBlock", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_decoding_error = { "DecodingError", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_uncaught_thread_exception = { "ThreadException", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_no_threading = { "NoThreading", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_internal_error = { "InternalError", &hlt_exception_unspecified, &hlt_type_info_hlt_string };
hlt_exception_type hlt_exception_os_error = { "OSError", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_overlay_not_attached = { "OverlayNotAttached", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_index_error = { "IndexError", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_underflow = { "Underflow", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_invalid_iterator = { "InvalidIterator", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_not_implemented = { "NotImplemented", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_pattern_error = { "PatternError", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_assertion_error = { "AssertionError", &hlt_exception_unspecified, &hlt_type_info_hlt_string };
hlt_exception_type hlt_exception_null_reference = { "NulLReference", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_timer_already_scheduled = { "TimerAlreadyScheduled", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_timer_not_scheduled = { "TimerNotScheduled", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_no_timer_manager = { "NoTimerManager", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_iosrc_exhausted = { "IOSrcExhausted", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_iosrc_error = { "IOSrcError", &hlt_exception_unspecified, &hlt_type_info_hlt_string };
hlt_exception_type hlt_exception_io_error = { "IOError", &hlt_exception_unspecified, &hlt_type_info_hlt_string };
hlt_exception_type hlt_exception_profiler_mismatch = { "ProfilerMismatch", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_profiler_unknown = { "ProfilerUnknown", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_no_thread_context = { "NoThreadContext", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_conversion_error = { "ConversionError", &hlt_exception_unspecified, 0 };

hlt_exception_type hlt_exception_resumable = { "Resumable", &hlt_exception_unspecified, 0 };
hlt_exception_type hlt_exception_yield = { "Yield", &hlt_exception_resumable, 0}; // FIXME: &hlt_type_info_hlt_int_32 };

void hlt_exception_dtor(hlt_type_info* ti, hlt_exception* excpt)
{
    if ( excpt->arg ) {
        GC_DTOR_GENERIC(excpt->arg, *excpt->type->argtype);
        hlt_free(excpt->arg);
    }

    if ( excpt->fiber )
        hlt_fiber_delete(excpt->fiber);
}

hlt_exception* hlt_exception_new(hlt_exception_type* type, void* arg, const char* location)
{
    hlt_exception* excpt = GC_NEW(hlt_exception);
    excpt->type = type;

    if ( arg ) {
        excpt->arg = hlt_malloc((*type->argtype)->size);
        memcpy(excpt->arg, arg, (*type->argtype)->size);
        GC_CCTOR_GENERIC(excpt->arg, *type->argtype);
    }

    else
        arg = 0;

    excpt->fiber = 0;
    excpt->vid = HLT_VID_MAIN;
    excpt->location = location;
    return excpt;
}

hlt_exception* hlt_exception_new_yield(hlt_fiber* fiber, const char* location)
{
   hlt_exception* excpt = hlt_exception_new(&hlt_exception_yield, 0, location);
   excpt->fiber = fiber;
   return excpt;
}

void* hlt_exception_arg(hlt_exception* excpt)
{
    return excpt->arg;
}

hlt_fiber* __hlt_exception_fiber(hlt_exception* excpt)
{
    return excpt->fiber;
}

void __hlt_exception_clear_fiber(hlt_exception* excpt)
{
    excpt->fiber = 0;
}

#if 0
hlt_exception* __hlt_exception_new_yield(hlt_continuation* cont, int32_t arg, const char* location)
{
    int32_t *arg_copy = hlt_malloc(sizeof(int32_t));
    *arg_copy = arg;
    hlt_exception *excpt = __hlt_exception_new(&hlt_exception_yield, arg_copy, location);
    excpt->cont = cont;
    return excpt;
}
#endif

void __hlt_set_exception(hlt_exception** dst, hlt_exception_type* type, void* arg, const char* location)
{
    assert(dst);
    GC_ASSIGN_REFED(*dst, hlt_exception_new(type, arg, location), hlt_exception);
}

int8_t __hlt_exception_match(hlt_exception* excpt, hlt_exception_type* type)
{
    if ( ! excpt )
        return 0;

    if ( ! type )
        return 1;

    for ( hlt_exception_type* t = excpt->type; t; t = t->parent ) {
        if ( t == type )
            return 1;
    }

    return 0;
}

static void __exception_print(const char* prefix, hlt_exception* exception, hlt_execution_context* ctx)
{
    hlt_exception* excpt = 0;

    // We must not terminate while in here.
    int old_state;
    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

    flockfile(stderr);
    fprintf(stderr, "%s%s", prefix, exception->type->name);

    if ( exception->arg ) {
        hlt_string arg = hlt_string_from_object(*exception->type->argtype, exception->arg, &excpt, ctx);
        fprintf(stderr, " with argument ");
        hlt_string_print(stderr, arg, 0, &excpt, ctx);
        GC_DTOR(arg, hlt_string);
    }

//    if ( exception->cont )
//        fprintf(stderr, ", resumable");

    if ( exception->vid != HLT_VID_MAIN )
        fprintf(stderr, " in virtual thread %" PRId64, exception->vid);

    if ( exception->location )
        fprintf(stderr, " (from %s)", exception->location);

    fprintf(stderr, "\n");

    fflush(stderr);
    funlockfile(stderr);

    hlt_pthread_setcancelstate(old_state, NULL);
}

void hlt_exception_print(hlt_exception* exception, hlt_execution_context* ctx)
{
    __exception_print("", exception, ctx);
}

void hlt_exception_print_uncaught(hlt_exception* exception, hlt_execution_context* ctx)
{
    __exception_print("hilti: uncaught exception, ", exception, ctx);
}

void hlt_exception_print_uncaught_in_thread(hlt_exception* exception, hlt_execution_context* ctx)
{
    __exception_print("hilti: uncaught exception in worker thread, ", exception, ctx);
}

void __hlt_exception_print_uncaught_abort(hlt_exception* exception, hlt_execution_context* ctx)
{
    __exception_print("hilti: uncaught exception, ", exception, ctx);
    hlt_abort();
}

