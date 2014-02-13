
#include "callable.h"
#include "int.h"
#include "string_.h"
#include "timer.h"
#include "vector.h"
#include "map_set.h"
#include "autogen/hilti-hlt.h"

#include "3rdparty/libpqueue/src/pqueue.h"

// The types of timers we have.
#define HLT_TIMER_FUNCTION 1
#define HLT_TIMER_MAP      2
#define HLT_TIMER_SET      3
#define HLT_TIMER_LIST     4
#define HLT_TIMER_VECTOR   5
#define HLT_TIMER_PROFILER 6

__HLT_DECLARE_RTTI_GC_TYPE(__hlt_map_timer_cookie)
__HLT_DECLARE_RTTI_GC_TYPE(__hlt_set_timer_cookie)

struct __hlt_timer_mgr {
    __hlt_gchdr __gchdr; // Header for memory management.
    hlt_time time;       // The current time.
    pqueue_t* timers;    // Priority list of all timers.
};

void hlt_timer_dtor(hlt_type_info* ti, hlt_timer* timer)
{
    switch (timer->type) {
      case HLT_TIMER_FUNCTION:
        GC_DTOR(timer->cookie.function, hlt_callable);
        break;

      case HLT_TIMER_LIST:
        GC_DTOR(timer->cookie.list, hlt_iterator_list);
        break;

      case HLT_TIMER_MAP:
        GC_DTOR(timer->cookie.map, __hlt_map_timer_cookie);
        break;

      case HLT_TIMER_SET:
        GC_DTOR(timer->cookie.set, __hlt_set_timer_cookie);
        break;

      case HLT_TIMER_VECTOR:
        GC_DTOR(timer->cookie.vector, hlt_iterator_vector);
        break;

      case HLT_TIMER_PROFILER:
        GC_DTOR(timer->cookie.profiler, hlt_string);
        break;

      default:
        abort();
    }
}

void hlt_timer_mgr_dtor(hlt_type_info* ti, hlt_timer_mgr* mgr)
{
    hlt_timer_mgr_expire(mgr, 0, 0, 0);
    pqueue_free(mgr->timers);
}

static void __hlt_timer_fire(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! timer->mgr )
        return;

    switch (timer->type) {
      case HLT_TIMER_FUNCTION:
        HLT_CALLABLE_RUN(timer->cookie.function, 0, Hilti_CallbackTimer, excpt, ctx);
        break;

      case HLT_TIMER_LIST:
        hlt_list_expire(timer->cookie.list);
        break;

      case HLT_TIMER_MAP:
        hlt_map_expire(timer->cookie.map);
        break;

      case HLT_TIMER_SET:
        hlt_set_expire(timer->cookie.set);
        break;

      case HLT_TIMER_VECTOR:
        hlt_vector_expire(timer->cookie.vector);
        break;

      case HLT_TIMER_PROFILER:
        hlt_profiler_timer_expire(timer->cookie.profiler, excpt, ctx);
        break;

      default:
        hlt_set_exception(excpt, &hlt_exception_internal_error, 0);
    }

    timer->mgr = 0;
    GC_DTOR(timer, hlt_timer);
}

hlt_timer* __hlt_timer_new_function(hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_FUNCTION;
    GC_INIT(timer->cookie.function, func, hlt_callable);
    return timer;
}

hlt_timer* __hlt_timer_new_list(__hlt_list_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_LIST;
    timer->cookie.list = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_vector(__hlt_vector_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_VECTOR;
    timer->cookie.vector = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_map(__hlt_map_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_MAP;
    timer->cookie.map = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_set(__hlt_set_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_SET;
    timer->cookie.set = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_profiler(__hlt_profiler_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) GC_NEW(hlt_timer);
    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_PROFILER;
    GC_INIT(timer->cookie.profiler, cookie, hlt_string);
    return timer;
}

hlt_time hlt_timer_time(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return timer->time;
}

void hlt_timer_update(hlt_timer* timer, hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    if ( ! timer->mgr ) {
        hlt_set_exception(excpt, &hlt_exception_timer_not_scheduled, 0);
        return;
    }

    if ( t > timer->mgr->time ) {
        timer->time = t;
        pqueue_change_priority(timer->mgr->timers, t, timer);
    }

    else {
        pqueue_remove(timer->mgr->timers, timer);
        __hlt_timer_fire(timer, excpt, ctx);
    }
}

void hlt_timer_cancel(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    pqueue_remove(timer->mgr->timers, timer);
    GC_DTOR(timer, hlt_timer);

    timer->mgr = 0;
    timer->time = 0;
}

hlt_string hlt_timer_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* timer = *((hlt_timer **)obj);

    if ( ! timer )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    if ( timer->time == HLT_TIME_UNSET )
        return hlt_string_from_asciiz("<unscheduled timer>", excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<timer scheduled at ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);

    hlt_string s = hlt_time_to_string(&hlt_type_info_hlt_time, &timer->time, options, excpt, ctx);
    hlt_string t = hlt_string_concat(prefix, s, excpt, ctx);
    hlt_string u = hlt_string_concat(t, postfix, excpt, ctx);

    GC_DTOR(s, hlt_string);
    GC_DTOR(t, hlt_string);
    GC_DTOR(prefix, hlt_string);
    GC_DTOR(postfix, hlt_string);

    return u;
}

double hlt_timer_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer **)obj);
    return hlt_time_to_double(&hlt_type_info_hlt_time, &t->time, options, excpt, ctx);
}

int64_t hlt_timer_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer **)obj);
    return hlt_time_to_int64(&hlt_type_info_hlt_time, &t->time, options, excpt, ctx);
}

hlt_timer_mgr* hlt_timer_mgr_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer_mgr* mgr = GC_NEW(hlt_timer_mgr);

    mgr->timers = pqueue_init(100);

    if ( ! mgr->timers ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    return mgr;
}

void hlt_timer_mgr_schedule(hlt_timer_mgr* mgr, hlt_time t, hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mgr ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    if ( timer->mgr ) {
        hlt_set_exception(excpt, &hlt_exception_timer_already_scheduled, 0);
        return;
    }

    timer->mgr = mgr; // Not memory managed to avoid cycles.
    timer->time = t;
    GC_CCTOR(timer, hlt_timer);

    if ( t <= mgr->time ) {
        __hlt_timer_fire(timer, excpt, ctx);
        return;
    }

    if ( pqueue_insert(mgr->timers, timer) != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
}

int32_t hlt_timer_mgr_advance(hlt_timer_mgr* mgr, hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mgr ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    int32_t count = 0;

    mgr->time = t;

    while ( 1 ) {
        hlt_timer* timer = (hlt_timer*) pqueue_peek(mgr->timers);

        if ( ! timer || timer->time > t )
            break;

        pqueue_pop(mgr->timers);

        __hlt_timer_fire(timer, excpt, ctx);

        ++count;
    }

    return count;
}

hlt_time hlt_timer_mgr_current(hlt_timer_mgr* mgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mgr ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    return mgr->time;
}

void hlt_timer_mgr_expire(hlt_timer_mgr* mgr, int8_t fire, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mgr ) {
        assert(excpt);
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    while ( 1 ) {
        hlt_timer* timer = (hlt_timer*) pqueue_pop(mgr->timers);
        if ( ! timer )
            break;

        if ( fire )
            __hlt_timer_fire(timer, excpt, ctx);
        else
            GC_DTOR(timer, hlt_timer);
    }
}

hlt_string hlt_timer_mgr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);

    if ( ! mgr )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    int64_t size = pqueue_size(mgr->timers);

    hlt_string size_str = hlt_int_to_string(&hlt_type_info_hlt_int_64, &size, options, excpt, ctx);
    hlt_string time_str = hlt_time_to_string(&hlt_type_info_hlt_time, &mgr->time, options, excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<timer_mgr at ", excpt, ctx);
    hlt_string middle = hlt_string_from_asciiz(" / ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(" active timers>", excpt, ctx);

    hlt_string s = hlt_string_concat(prefix, time_str, excpt, ctx);
    hlt_string t = hlt_string_concat(s, middle, excpt, ctx);
    hlt_string u = hlt_string_concat(t, size_str, excpt, ctx);
    hlt_string v = hlt_string_concat(u, postfix, excpt, ctx);

    GC_DTOR(size_str, hlt_string);
    GC_DTOR(time_str, hlt_string);
    GC_DTOR(prefix, hlt_string);
    GC_DTOR(middle, hlt_string);
    GC_DTOR(postfix, hlt_string);
    GC_DTOR(s, hlt_string);
    GC_DTOR(t, hlt_string);
    GC_DTOR(u, hlt_string);

    return v;
}

double hlt_timer_mgr_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);
    return hlt_time_to_double(&hlt_type_info_hlt_time, &mgr->time, options, excpt, ctx);
}

int64_t hlt_timer_mgr_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);
    return hlt_time_to_int64(&hlt_type_info_hlt_time, &mgr->time, options, excpt, ctx);
}
