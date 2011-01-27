// $Id$

#include "hilti.h"
#include "continuation.h"

#include "3rdparty/libpqueue/src/pqueue.h"

// The types of timers we have.
#define HLT_TIMER_FUNCTION 1
#define HLT_TIMER_LIST     2
#define HLT_TIMER_MAP      3
#define HLT_TIMER_SET      4
#define HLT_TIMER_PROFILER 5

struct __hlt_timer_mgr {
    hlt_time time;      // The current time.
    pqueue_t* timers;   // Priority list of all timers.
};

static void __hlt_timer_fire(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx)
{
    switch (timer->type) {
      case HLT_TIMER_FUNCTION:
        hlt_callable_register(timer->cookie.function, excpt, ctx);
        break;

#if 0
      case HLT_TIMER_LIST:
        hlt_list_iter_expire(timer->cookie.list, excpt, ctx);
        break;
#endif

      case HLT_TIMER_MAP:
        hlt_list_map_expire(timer->cookie.map);
        break;

      case HLT_TIMER_SET:
        hlt_list_set_expire(timer->cookie.set);
        break;

      case HLT_TIMER_PROFILER:
        hlt_profiler_timer_expire(timer->cookie.profiler, excpt, ctx);
        break;

      default:
        hlt_set_exception(excpt, &hlt_exception_internal_error, 0);
    }
}

hlt_timer* __hlt_timer_new_function(hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer));
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_FUNCTION;
    timer->cookie.function = func;
    return timer;
}


#if 0
hlt_timer* __hlt_timer_new_list(__hlt_list_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer));
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_LIST;
    timer->cookie.list = cookie;
    return timer;
}
#endif

hlt_timer* __hlt_timer_new_map(__hlt_map_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer));
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_MAP;
    timer->cookie.map = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_set(__hlt_set_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer));
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_SET;
    timer->cookie.set = cookie;
    return timer;
}

hlt_timer* __hlt_timer_new_profiler(__hlt_profiler_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer* timer = (hlt_timer*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer));
    if ( ! timer ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    timer->mgr = 0;
    timer->time = HLT_TIME_UNSET;
    timer->type = HLT_TIMER_PROFILER;
    timer->cookie.profiler = cookie;
    return timer;
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

    if ( t > timer->mgr->time )
        pqueue_change_priority(timer->mgr->timers, t, timer);

    else {
        hlt_timer_cancel(timer, excpt, ctx);
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
    timer->mgr = 0;
    timer->time = 0;
}

static hlt_string_constant timer_prefix = HLT_STRING_CONSTANT("<timer scheduled at ");
static hlt_string_constant timer_postfix = HLT_STRING_CONSTANT(">");
static hlt_string_constant timer_unsched = HLT_STRING_CONSTANT("<unscheduled timer>");

extern const hlt_type_info hlt_type_info_string;
extern const hlt_type_info hlt_type_info_double;
extern const hlt_type_info hlt_type_info_int_64;
extern const hlt_type_info hlt_type_info_time;

hlt_string hlt_timer_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer **)obj);

    if ( t->time == HLT_TIME_UNSET )
        return &timer_unsched;

    hlt_string s = hlt_time_to_string(&hlt_type_info_time, &t->time, options, excpt, ctx);
    s = hlt_string_concat(&timer_prefix, s, excpt, ctx);
    return hlt_string_concat(s, &timer_postfix, excpt, ctx);
}

double hlt_timer_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer **)obj);
    return hlt_time_to_double(&hlt_type_info_time, &t->time, options, excpt, ctx);
}

int64_t hlt_timer_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER);
    hlt_timer* t = *((hlt_timer **)obj);
    return hlt_time_to_int64(&hlt_type_info_time, &t->time, options, excpt, ctx);
}

hlt_timer_mgr* hlt_timer_mgr_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_timer_mgr* mgr = (hlt_timer_mgr*) hlt_gc_malloc_non_atomic(sizeof(hlt_timer_mgr));
    if ( ! mgr ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

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

    if ( t <= mgr->time ) {
        __hlt_timer_fire(timer, excpt, ctx);
        return;
    }

    timer->mgr = mgr;
    timer->time = t;

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

    while ( true ) {
        hlt_timer* timer = (hlt_timer*) pqueue_peek(mgr->timers);
        if ( ! timer || timer->time > t )
            break;

        __hlt_timer_fire(timer, excpt, ctx);
        pqueue_pop(mgr->timers);
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
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    if ( ! fire ) {
        // Just throw them all away and let GC do the rest.
        mgr->timers = pqueue_init(100);
        return;
    }

    while ( true ) {
        hlt_timer* timer = (hlt_timer*) pqueue_pop(mgr->timers);
        if ( ! timer )
            break;

        __hlt_timer_fire(timer, excpt, ctx);
    }
}

static hlt_string_constant tmgr_prefix = HLT_STRING_CONSTANT("<timer_mgr at ");
static hlt_string_constant tmgr_middle = HLT_STRING_CONSTANT(" / ");
static hlt_string_constant tmgr_postfix = HLT_STRING_CONSTANT(" active timers>");

hlt_string hlt_timer_mgr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);

    int64_t size = pqueue_size(mgr->timers);
    hlt_string size_str = hlt_int_to_string(&hlt_type_info_int_64, &size, options, excpt, ctx);

    hlt_string s = hlt_time_to_string(&hlt_type_info_time, &mgr->time, options, excpt, ctx);
    s = hlt_string_concat(&tmgr_prefix, s, excpt, ctx);
    s = hlt_string_concat(s, &tmgr_middle, excpt, ctx);
    s = hlt_string_concat(s, size_str, excpt, ctx);
    return hlt_string_concat(s, &tmgr_postfix, excpt, ctx);
}

double hlt_timer_mgr_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);
    return hlt_time_to_double(&hlt_type_info_time, &mgr->time, options, excpt, ctx);
}

int64_t hlt_timer_mgr_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIMER_MGR);
    hlt_timer_mgr* mgr = *((hlt_timer_mgr **)obj);
    return hlt_time_to_int64(&hlt_type_info_time, &mgr->time, options, excpt, ctx);
}
