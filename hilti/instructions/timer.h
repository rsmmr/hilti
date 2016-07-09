///
/// \type Timer
///
/// A ``timer`` executes an action at a certain point of time in the future,
/// where "time" is defined by a ``timer_mgr`` object with which each timer
/// is associated. Each manager tracks its time as a monotonically increasing
/// ``time`` value. Managers never advance their time themselves; the HILTI
/// program does that explicitly.
///
/// \cproto hlt_timer*
///
/// \type Timer Manager
///
/// TODO.
///
/// \cproto hlt_timer_mgr*

#include "define-instruction.h"

iBegin(timer, New, "new")
    iTarget(optype::refTimer);
    iOp1(optype::typeTimer, true);
    iOp2(optype::function, true);
    iOp3(optype::tuple, true);

    iValidate
    {
        // TODO: Check arguments.
    }

    iDoc(R"(
         Instantiates a new timer that upon expiration will run function *op2*
         with arguments *op3*. The arguments are evaluated at the time when the
         schedule instruction is executed. The called function must not return a
         value.
    )")

iEnd


iBegin(timer, Cancel, "timer.cancel")
    iOp1(optype::refTimer, false);

    iValidate
    {
    }

    iDoc(R"(    
        Cancels the timer *op1*. This also detaches the timer from its
        manager, and allows it to be rescheduled subsequently.
    )")

iEnd

iBegin(timer, Update, "timer.update")
    iOp1(optype::refTimer, false);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Adjusts *op1*'s expiration time to *op2*.  Raises
        ``TimerNotScheduled`` if the timer is not scheduled with a timer
        manager.
    )")

iEnd
