/// \type Timer Manager
///
/// TODO.
/// 
/// \cproto hlt_timer_mgr*

#include "instructions/define-instruction.h"

iBegin(timer_mgr, New, "new")
    iTarget(optype::refTimerMgr)
    iOp1(optype::typeTimerMgr, true);

    iValidate {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``timer_mgr`` object.
    )")

iEnd


iBegin(timer_mgr, Advance, "timer_mgr.advance")
    iOp1(optype::refTimerMgr, false)
    iOp2(optype::time, true)

    iValidate {
    }

    iDoc(R"(    
        Advances *op1*'s notion of time to *op2*. Time can never go backwards,
        and thus the instruction has no effect if *op2* is smaller than the
        timer manager's current time.
    )")

iEnd

iBegin(timer_mgr, Current, "timer_mgr.current")
    iTarget(optype::time)
    iOp1(optype::refTimerMgr, true)

    iValidate {
    }

    iDoc(R"(    
        Returns *op1*'s current time.
    )")

iEnd

iBegin(timer_mgr, Expire, "timer_mgr.expire")
    iOp1(optype::refTimerMgr, false)
    iOp2(optype::boolean, true)

    iValidate {
    }

    iDoc(R"(    
        Expires all timers currently associated with *op1*. If *op2* is True,
        all their actions will be executed immediately; if it's False, all the
        timers will simply be canceled.
    )")

iEnd

iBegin(timer_mgr, Schedule, "timer_mgr.schedule")
    iOp1(optype::refTimerMgr, false)
    iOp2(optype::time, true)
    iOp3(optype::refTimer, false)

    iValidate {
    }

    iDoc(R"(    
        Schedules *op2* with the timer manager *op1* to be executed at time
        *op2*. If *op2* is smaller or equal to the manager's current time, the
        timer fires immediately. Each timer can only be scheduled with a
        single time manager at a time; it needs to be canceled before it can
        be rescheduled.  Raises ``TimerAlreadyScheduled`` if the timer is
        already scheduled with a timer manager.
    )")

iEnd

