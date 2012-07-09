
iBeginH(profiler, Start, "profiler.start")
    iOp1(optype::string, true)
    iOp2(optype::any, true)
    iOp3(optype::optional(optype::refTimerMgr), true);
iEndH

iBeginH(profiler, Stop, "profiler.stop")
    iOp1(optype::string, true)
iEndH

iBeginH(profiler, Update, "profiler.update")
    iOp1(optype::string, true)
    iOp2(optype::optional(optype::int64), true);
iEndH
