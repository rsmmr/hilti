
#include "timer.h"
#include "autogen/hilti-hlt.h"
#include "callable.h"
#include "int.h"
#include "map_set.h"
#include "string_.h"
#include "vector.h"

#include "3rdparty/libpqueue/src/pqueue.h"

// The types of timers we have.
#define HLT_TIMER_FUNCTION 1
#define HLT_TIMER_MAP 2
#define HLT_TIMER_SET 3
#define HLT_TIMER_LIST 4
#define HLT_TIMER_VECTOR 5
#define HLT_TIMER_PROFILER 6

struct __hlt_timer_mgr {
    __hlt_gchdr __gchdr;      // Header for memory management.
    hlt_time time;            // The current time.
    priority_queue_t* timers; // Priority list of all timers.
};

void hlt_timer_dtor(hlt_type_info* ti, hlt_timer* timer, hlt_execution_context* ctx)
{
    switch ( timer->type ) {
    case HLT_TIMER_FUNCTION:
        GC_DTOR(timer->cookie.function, hlt_callable, ctx);
        break;

    case HLT_TIMER_LIST:
        GC_DTOR(timer->cookie.list, hlt_iterator_list, ctx);
        break;

    case HLT_TIMER_MAP:
        // Nothing to do.
        break;

    case HLT_TIMER_SET:
        // Nothing to do.
        break;

    case HLT_TIMER_VECTOR:
        GC_DTOR(timer->cookie.vector, hlt_iterator_vector, ctx);
        break;

    case HLT_TIMER_PROFILER:
        GC_DTOR(timer->cookie.profiler, hlt_string, ctx);
        break;

    default:
        abort();
    }
}

void hlt_timer_mgr_dtor(hlt_type_info* ti, hlt_timer_mgr* mgr, hlt_execution_context* ctx)
{
    hlt_exception* excpt = 0;
    hlt_timer_mgr_expire(mgr, 0, &excpt, ctx);
    priority_queue_free(mgr->timers);
}

static void __hlt_timer_fire(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! timer->mgr )
        return;

    switch ( timer->type ) {
    case HLT_TIMER_FUNCTION:
        HLT_CALLABLE_RUN(timer->cookie.function, 0, Hilti_CallbackTimer, excpt, ctx);
        break;

    case HLT_TIMER_LIST:
        hlt_list_expire(timer->cookie.list, excpt, ctx);
        break;

    case HLT_TIMER_MAP:
        hlt_map_expire(timer->cookie.map, excpt, ctx);
        break;

    case HLT_TIMER_SET:
        hlt_set_expire(timer->cookie.set, excpt, ctx);
        break;

    case HLT_TIMER_VECTOR:
        hlt_vector_expire(timer->cookie.vector, excpt, ctx);
        break;

    case HLT_TIMER_PROFILER:
        hlt_profiler_timer_expire(timer->cookie.profiler, excpt, ctx);
        break;

    default:
        hlt_set_exception(excpt, &hlt_exception_internal_error, 0, ctx);
    }

    timer->mgr = 0;
    GC_DTOR(timer, hlt_timer, ctx);
}

hlt_timer* __hlt_timer_new_function(hlt_callable* func, hlt_exception** excpt,
                                    hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_FUNCTION;
    GC_INIT(timer->cookie.function, func, hlt_callable, ctx);
    return timer;
}

hlt_timer* __hlt_timer_new_list(__hlt_list_timer_cookie cookie, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_LIST;
    timer->cookie.list = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_vector(__hlt_vector_timer_cookie cookie, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_VECTOR;
    timer->cookie.vector = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_map(__hlt_map_timer_cookie cookie, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_MAP;
    timer->cookie.map = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_set(__hlt_set_timer_cookie cookie, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_SET;
    timer->cookie.set = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_profiler(__hlt_profiler_timer_cookie cookie, hlt_exception** excpt,
                                    hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*)GC_NEW(hlt_timer, ctx);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_PROFILER;
    GC_INIT(timer->cookie.profiler, cookie, hlt_string, ctx);
    return timer;
}

hlt_time hlt_timer_time(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return timer->time;
}

void hlt_timer_update(hlt_timer* timer, hlt_time t, hlt_exception** excpt,
                      hlt_execution_context* ctx)
{
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    if ( ! timer->mgr ) {
        hlt_set_exception(excpt, &hlt_exception_timer_not_scheduled, 0, ctx);
        return;
    }

    if ( t > timer->mgr->time ) {
        timer->time = t;
        priority_queue_change_priority(timer->mgr->timers, t, timer);
    }

    else {
        priority_queue_remove(timer->mgr->timers, timer);
        __hlt_timer_fire(timer, excpt, ctx);
    }
}

void hlt_timer_cancel(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    priority_queue_remove(timer->mgr->timers, timer);
    GC_DTOR(timer, hlt_timer, ctx);

    timer->mgr = 0;
    timer->time = 0;
}

hlt_string hlt_timer_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                               __hlt_pointer_stack* seen, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* timer = *((hlt_timer**)obj);

    if ( ! timer )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    if ( timer->time == HLT_TIME_UNSET )
        return hlt_string_from_asciiz("<unscheduled timer>", excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<timer scheduled at ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);

    hlt_string s =
        hlt_time_to_string(&hlt_type_info_hlt_time, &timer->time, options, seen, excpt, ctx);
    hlt_string t = hlt_string_concat(prefix, s, excpt, ctx);
    hlt_string u = hlt_string_concat(t, postfix, excpt, ctx);

    return u;
}

double hlt_timer_to_double(const hlt_type_info* type, const void* obj, int32_t options,
                           hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer**)obj);
    return hlt_time_to_double(&hlt_type_info_hlt_time, &t->time, options, excpt, ctx);
}

int64_t hlt_timer_to_int64(const hlt_type_info* type, const void* obj, int32_t options,
                           hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer**)obj);
    return hlt_time_to_int64(&hlt_type_info_hlt_time, &t->time, options, excpt, ctx);
}

void hlt_timer_mgr_advance_global(hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ctx->vid != HLT_VID_MAIN ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return;
    }

    hlt_timer_mgr_advance(ctx->tmgr, t, excpt, ctx);

    if ( t > __hlt_globals()->global_time )
        __hlt_globals()->global_time = t;
}

hlt_timer_mgr* hlt_timer_mgr_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer_mgr* mgr = GC_NEW(hlt_timer_mgr, ctx);

    mgr->timers = priority_queue_init(100);

    if ( ! mgr->timers ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0, ctx);
        return 0;
    }

    return mgr;
}

void hlt_timer_mgr_schedule(hlt_timer_mgr* mgr, hlt_time t, hlt_timer* timer, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    if ( timer->mgr ) {
        hlt_set_exception(excpt, &hlt_exception_timer_already_scheduled, 0, ctx);
        return;
    }

    if ( ! mgr )
        mgr = ctx->tmgr;

    timer->mgr = mgr; // Not memory managed to avoid cycles.
    timer->time = t;
    GC_CCTOR(timer, hlt_timer, ctx);

    if ( t <= mgr->time ) {
        __hlt_timer_fire(timer, excpt, ctx);
        return;
    }

    if ( priority_queue_insert(mgr->timers, timer) != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0, ctx);
        return;
    }
}

int32_t hlt_timer_mgr_advance(hlt_timer_mgr* mgr, hlt_time t, hlt_exception** excpt,
                              hlt_execution_context* ctx)
{
    if ( ! mgr )
        mgr = ctx->tmgr;

    assert(mgr);

    int32_t count = 0;

    mgr->time = t;

    while ( 1 ) {
        hlt_timer* timer = (hlt_timer*)priority_queue_peek(mgr->timers);

        if ( ! timer || timer->time > t )
            break;

        priority_queue_pop(mgr->timers);

        __hlt_timer_fire(timer, excpt, ctx);

        ++count;
    }

    return count;
}

hlt_time hlt_timer_mgr_current(hlt_timer_mgr* mgr, hlt_exception** excpt,
                               hlt_execution_context* ctx)
{
    if ( ! mgr )
        mgr = ctx->tmgr;

    return mgr->time;
}

void hlt_timer_mgr_expire(hlt_timer_mgr* mgr, int8_t fire, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! mgr ) {
        assert(ctx);
        mgr = ctx->tmgr;
    }

    while ( 1 ) {
        hlt_timer* timer = (hlt_timer*)priority_queue_pop(mgr->timers);
        if ( ! timer )
            break;

        if ( fire )
            __hlt_timer_fire(timer, excpt, ctx);
        else
            GC_DTOR(timer, hlt_timer, ctx);
    }
}

hlt_string hlt_timer_mgr_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                                   __hlt_pointer_stack* seen, hlt_exception** excpt,
                                   hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr**)obj);

    if ( ! mgr )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    int64_t size = priority_queue_size(mgr->timers);

    hlt_string size_str =
        hlt_int_to_string(&hlt_type_info_hlt_int_64, &size, options, seen, excpt, ctx);
    hlt_string time_str =
        hlt_time_to_string(&hlt_type_info_hlt_time, &mgr->time, options, seen, excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<timer_mgr at ", excpt, ctx);
    hlt_string middle = hlt_string_from_asciiz(" / ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(" active timers>", excpt, ctx);

    hlt_string s = hlt_string_concat(prefix, time_str, excpt, ctx);
    hlt_string t = hlt_string_concat(s, middle, excpt, ctx);
    hlt_string u = hlt_string_concat(t, size_str, excpt, ctx);
    hlt_string v = hlt_string_concat(u, postfix, excpt, ctx);

    return v;
}

double hlt_timer_mgr_to_double(const hlt_type_info* type, const void* obj, int32_t options,
                               hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr**)obj);
    return hlt_time_to_double(&hlt_type_info_hlt_time, &mgr->time, options, excpt, ctx);
}

int64_t hlt_timer_mgr_to_int64(const hlt_type_info* type, const void* obj, int32_t options,
                               hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr**)obj);
    return hlt_time_to_int64(&hlt_type_info_hlt_time, &mgr->time, options, excpt, ctx);
}
