/// \type Timer Manager
///
/// TODO.
///
/// \cproto hlt_timer_mgr*

#include "define-instruction.h"

iBegin(timer_mgr, New, "new")
    iTarget(optype::refTimerMgr);
    iOp1(optype::typeTimerMgr, true);

    iValidate
    {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``timer_mgr`` object.
    )")

iEnd


iBegin(timer_mgr, Advance, "timer_mgr.advance")
    iOp1(optype::time, true);
    iOp2(optype::optional(optype::refTimerMgr), false);

    iValidate
    {
    }

    iDoc(R"(    
        Advances *op2*'s notion of time to *op1*. If *op2* is omitted, defaults
        to the current thread's timer manager. Time can never go backwards,
        and thus the instruction has no effect if *op1* is smaller than the
        timer manager's current time.
    )")
iEnd

iBegin(timer_mgr, AdvanceGlobal, "timer_mgr.advance_global")
    iOp1(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Advances the global notion of time to *op1*.
        This will advance all threads' timer managers to *op1*. Time can never go backwards,
        and thus the instruction has no effect on a thread if *op1* is smaller than a
        thread's current time.
        This instruction must be called only from the main thread.
    )")
iEnd

iBegin(timer_mgr, Current, "timer_mgr.current")
    iTarget(optype::time);
    iOp1(optype::optional(optype::refTimerMgr), true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns *op1*'s current time.
        If *op1* is omitted, defaults to the current thread's timer manager
    )")

iEnd

iBegin(timer_mgr, Expire, "timer_mgr.expire")
    iOp1(optype::boolean, true);
    iOp2(optype::optional(optype::refTimerMgr), false);

    iValidate
    {
    }

    iDoc(R"(    
        Expires all timers currently associated with *op2*.
        If *op2* is omitted, defaults to the current thread's timer manager
        If *op1* is True,
        all their actions will be executed immediately; if it's False, all the
        timers will simply be canceled.
    )")

iEnd

iBegin(timer_mgr, Schedule, "timer_mgr.schedule")
    iOp1(optype::time, true);
    iOp2(optype::refTimer, false);
    iOp3(optype::optional(optype::refTimerMgr), false);

    iValidate
    {
    }

    iDoc(R"(    
        Schedules *op2* with the timer manager *op3* to be executed at time
        *op1*.
        If *op3* is omitted, defaults to the current thread's timer manager
        If *op2* is smaller or equal to the manager's current time, the
        timer fires immediately. Each timer can only be scheduled with a
        single time manager at a time; it needs to be canceled before it can
        be rescheduled.  Raises ``TimerAlreadyScheduled`` if the timer is
        already scheduled with a timer manager.
    )")

iEnd
