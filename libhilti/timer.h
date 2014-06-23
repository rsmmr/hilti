// $Id$
///
/// \addtogroup timer
/// @{
/// Functions for manipulating timers and timer managers.
///
/// A HILTI timer is represented by ~~hlt_timer, and a timer manager by
/// ~~hlt_timer_mgr.
///
/// Timers are always associated with exactly one timer manager, and they
/// execute a specific action when the manager's time reaches the timers'
/// expiration time.
///
/// The action executed by a timer is determined at construction time by
/// choosing the corresponding ``hlt_timer_new_*`` function. Actions include
/// executing a bound function, and expiring container elements. We hard-code
/// the individual actions into the C code for efficiency, rather than using
/// indirection via some kind of generic mechanism.
///
/// Internally, timer managers use Fibonacci heaps to keep a priority list of
/// all their timers. That allows for efficient lookups and updating.
/// @}

#ifndef LIBHILTI_TIMER_H
#define LIBHILTI_TIMER_H

#include "time_.h"
#include "list.h"
#include "vector.h"
#include "map_set.h"
#include "context.h"
#include "exceptions.h"
#include "callable.h"
#include "profiler.h"

typedef struct __hlt_timer hlt_timer;       ///< Type for representing a HILTI timer.

// Todo: We store the timer manager with every timer. That's kind of a waste,
// but it makes handing timers around much easier. Need to recheck eventually
// whether that is the right trade-off.
struct __hlt_timer {
    __hlt_gchdr __gchdr; // Header for memory management.
    hlt_timer_mgr* mgr;  // The timer manager the timer belongs to. No memory-managed to avoid cycles.
    hlt_time time;       // Expiration time.
    size_t queue_pos;    // Used by priority queue.
    int16_t type;        // One of HLT_TIMER_* indicating the timer's type.
    union {              // The timer's payload cookie corresponding to its type.
        hlt_callable* function;
        __hlt_list_timer_cookie list;
        __hlt_map_timer_cookie map;
        __hlt_set_timer_cookie set;
        __hlt_vector_timer_cookie vector;
        __hlt_profiler_timer_cookie profiler;
    } cookie;
};

/// Returns a timer's expiration time.
///
/// timer: The timer to query.
///
/// excpt: &
extern hlt_time hlt_timer_time(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx);

/// Advances a timer to a time further ahead.
///
/// timer: The timer to cancel.
///
/// t: The new expiration time. If it's smaller than the timer managers
/// current time, the timer will be executed immediately.
///
/// excpt: &
///
/// Raises: NotScheduled - If the timer has not been scheduled yet.
extern void hlt_timer_update(hlt_timer* timer, hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx);

/// Cancels a timer without triggering it. This also detaches the timer from
/// the timer manager, and it can be rescheduled afterwards.
///
/// timer: The timer to cancel.
///
/// excpt: &
extern void hlt_timer_cancel(hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a timer to a string representian.
///
/// This function has the standard RTTI signature.
extern hlt_string hlt_timer_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Converts a timer to a double. The returned double is the timer's
/// expiration time in seconds since the epoch, or HLT_TIME_NONE if none has
/// been set.
///
/// This function has the standard RTTI signature.
extern double hlt_timer_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Converts a timer to an int64. The returned valye is the timer's
/// expiration time in nanoseconds since the epoch, or HLT_TIME_NONE if none
/// has been set.
///
/// This function has the standard RTTI signature.
extern int64_t hlt_timer_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Instantiates a new timer manager object. It's current time well initially be set to zero.
///
/// excpt: &
///
/// Returns: The new timer manager object.
extern hlt_timer_mgr* hlt_timer_mgr_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Schedules a timer with the timer manager. A timer can only be scheduled
/// with one timer manager at a time. It needs to be canceled before it can
/// be rescheduled.
///
/// mgr: The timer manager. If null, uses the context's manager.
///
/// t: The time when the timer is to be executed.  If *t* exceeds the
/// manager's current time, it will fire immediately.
///
/// timer: The timer to schedule.
///
/// excpt: &
///
/// Raises: TimerAlreadyScheduled - The timer is already associated with
/// another timer manager.
extern void hlt_timer_mgr_schedule(hlt_timer_mgr* mgr, hlt_time t, hlt_timer* timer, hlt_exception** excpt, hlt_execution_context* ctx);

/// Advances a the global notion of time that's synchronized across all
/// contexts. All timers scheduled for a time smaller or equal the new time
/// will fire.
///
/// This function must be called only from the main thread.
///
/// t: The new global point of time.
///
/// excpt: &
extern void hlt_timer_mgr_advance_global(hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx);

/// Advances a timer manager's notion of time. All timers scheduled for a
/// time smaller or equal the new time will fire.
///
/// mgr: The timer manager. If null, uses the context's manager.
///
/// t: The new point of time. If it's smaller than the current time, the
/// function call is ignored.
///
/// excpt: &
///
/// Returns: The number of timers which have fired.
extern int32_t hlt_timer_mgr_advance(hlt_timer_mgr* mgr, hlt_time t, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the timer managers current time.
///
/// mgr: The timer manager. If null, uses the context's manager.
///
/// excpt: &
///
/// Returns: The current time.
extern hlt_time hlt_timer_mgr_current(hlt_timer_mgr* mgr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Cancels all scheduled timer, optionally firing them all.
///
/// mgr: The timer manager. If null, uses the context's manager.
///
/// fire: If true, all timers are executed.
///
/// excpt: &
extern void hlt_timer_mgr_expire(hlt_timer_mgr* mgr, int8_t fire, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a timer manager to string representian.
///
/// This function has the standard RTTI signature.
extern hlt_string hlt_timer_mgr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Converts a timer manager to a double. The returned double is the
/// manager's current time in seconds since the epoch.
///
/// This function has the standard RTTI signature.
extern double hlt_timer_mgr_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Converts a timer manager to an int64. The returned value is the
/// manager's current time in nanoseconds since the epoch.
///
/// This function has the standard RTTI signature.
extern int64_t hlt_timer_mgr_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx);

/// Instantiates a new timer object that will execute a bound function when it
/// fires.
///
/// func: The function.
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_function(hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new timer object that will expire a list entry when it
/// fires.
///
/// cookie: A list-specific cookie to identify the item to be expired. The
/// function takes ownership (i.e., it must have been ref'ed already.)
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_list(__hlt_list_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new timer object that will expire a map entry when it
/// fires.
///
/// cookie: A map-specific cookie to identify the item to be expired. The
/// function takes ownership (i.e., it must have been ref'ed already.)
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_map(__hlt_map_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new timer object that will expire a vector entry when it
/// fires.
///
/// cookie: A vector-specific cookie to identify the item to be expired. The
/// function takes ownership (i.e., it must have been ref'ed already.)
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_vector(__hlt_vector_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new timer object that will expire a set entry when it
/// fires.
///
/// cookie: A set-specific cookie to identify the item to be expired. The
/// function takes ownership (i.e., it must have been ref'ed already.)
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_set(__hlt_set_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new timer object that will record a profiler snapshot when it
/// fires.
///
/// cookie: A profiler-specific cookie to identify the profiler to snapshot.
/// The function takes ownership (i.e., it must have been ref'ed already.)
///
/// excpt: &
///
/// Returns: The new timer object.
extern hlt_timer* __hlt_timer_new_profiler(__hlt_profiler_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx);


#endif
