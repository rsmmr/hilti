
#include "define-instruction.h"

#include "profiler.h"
#include "../module.h"

iBeginCC(profiler)
    iValidateCC(Start) {
        // FIXME: We need to check here that the enum is right, and that the
        // argument matches with what the profilter type expects.

        if ( ! op2 )
            return;

        if ( ! isConstant(op2) )
            // Error, must be constant (but is reported already).
            return;

        if ( ast::isA<type::Enum>(op2->type()) )
            // Ok.
            return;

        auto ttype = ast::tryCast<type::Tuple>(op2->type());

        if ( ! ttype )
            goto error;

        if ( ttype->typeList().size() == 2 ) {

            auto types = ttype->typeList();
            auto a = types.begin();
            auto arg1 = *a++;
            auto arg2 = *a++;

            if ( ! ast::isA<type::Enum>(arg1) )
                goto error;

            if ( ! (ast::isA<type::Integer>(arg2) || ast::isA<type::Interval>(arg2)) )
                goto error;

            // Ok.
            return;
        }

error:
        // Error when arriving here.
        error(op2, "profiler parameter must be Hilti::ProfileStyle or (Hilti::ProfileStyle, int64|interval)");
    }

    iDocCC(Start, R"(
        Starts collecting profiling with a profilter associated with tag
        *op1*, *op2* specifies a ``Hilti:: ProfileStyle``, defining when
        automatic snapshots should be taken of the profiler's counters; the
        first element of *op2* is the style, and the second its argument if
        one is needed (if not, the argument is simply ignored). If *op3* is
        given, it attaches a ``timer_mgr`` to the started profiling, which
        will then (1) record times according to the timer manager progress,
        and (2) can trigger snaptshots according to its time.  If a tag is
        given for which a profiler has already been started, this instruction
        increases the profiler's level by one, and ``profiler.stop`` command
        will only record information if it has been called a matching number
        of times.  If profiling support is not compiled in, this instruction
        is a no-op.
    )")
iEndCC

iBeginCC(profiler)
    iValidateCC(Stop) {
    }

    iDocCC(Stop, R"(
        Stops the profiler associated with the tag *op1*, recording its
        current state to disk. However, a profiler is only stopped if ``stop``
        has been called as often as it has previously been started with
        ``profile.start``.  If profiling support is not compiled in, this
        instruction is a no-op.
    )")

iEndCC

iBeginCC(profiler)
    iValidateCC(Update) {
    }

    iDocCC(Update, R"(
        Records a snapshot of the current state of the profiler associated
        with tag *op1* (in ``STANDARD`` mode, otherwise according to the
        style).  If *op2* is given, the profiler's user-defined counter is
        adjusted by that amount.  If profiling support is not compiled in,
        this instruction is a no-op.
    )")

iEndCC

