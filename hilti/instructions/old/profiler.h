iBegin(profiler, Start, "profiler.start")
    iOp1(optype::string, trueX)
    iOp2(optype::[Hilti::ProfileStyle | (Hilti::ProfileStyle, int<64>)], trueX)
    iOp3(optype::[ref\ <timer_mgr>], trueX)

    iValidate {
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::[Hilti::ProfileStyle | (Hilti::ProfileStyle, int<64>)]>(op2->type());
        auto ty_op3 = as<type::[ref\ <timer_mgr>]>(op3->type());

    }

    iDoc(R"(    
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

iEnd

iBegin(profiler, Stop, "profiler.stop")
    iOp1(optype::string, trueX)

    iValidate {
        auto ty_op1 = as<type::string>(op1->type());

    }

    iDoc(R"(    
        Stops the profiler associated with the tag *op1*, recording its
        current state to disk. However, a profiler is only stopped if ``stop``
        has been called as often as it has previously been started with
        ``profile.start``.  If profiling support is not compiled in, this
        instruction is a no-op.
    )")

iEnd

iBegin(profiler, Update, "profiler.update")
    iOp1(optype::string, trueX)
    iOp2(optype::[int\ <64>], trueX)

    iValidate {
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::[int\ <64>]>(op2->type());

    }

    iDoc(R"(    
        Records a snapshot of the current state of the profiler associated
        with tag *op1* (in ``STANDARD`` mode, otherwise according to the
        style).  If *op2* is given, the profiler's user-defined counter is
        adjusted by that amount.  If profiling support is not compiled in,
        this instruction is a no-op.
    )")

iEnd

