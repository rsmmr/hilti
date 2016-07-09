
iBegin(debug, Assert, "debug.assert")
    iOp1(optype::boolean, true);
    iOp2(optype::optional(optype::string), true);
    iHideInDebugTrace();

    iValidate
    {
    }

    iDoc(R"(    
        If compiled in debug mode, verifies that *op1* is true. If not, throws
        an ~~AssertionError exception with *op2* as its value if given. A no-op
        in release mode.
    )")


iEnd

iBegin(debug, InternalError, "debug.internal_error")
    iOp1(optype::string, true);

    iValidate
    {
    }

    iDoc(R"(    
        Throws an InternalError exception, setting *op1* as the its argument.
        Note: This is just a convenience instruction; one can also raise the
        exception directly.
    )")

iEnd

iBegin(debug, Msg, "debug.msg")
    iOp1(optype::string, true);
    iOp2(optype::string, true);
    iOp3(optype::tuple, true);
    iHideInDebugTrace();

    iValidate
    {
    }

    iDoc(R"(    
        If compiled in debug mode, prints a debug message to stderr. The
        message is only printed of the debug stream *op1* has been activated.
        *op2* is printf-style format string and *op3* the correponding
        arguments.
    )")

iEnd

iBegin(debug, PopIndent, "debug.pop_indent")
    iHideInDebugTrace();

    iValidate
    {
    }

    iDoc(R"(    
        Decreases the indentation in debugging output by one level.  Note: The
        indentation level is global across all debug streams, but separately
        tracked for each thread.
    )")

iEnd

iBegin(debug, PushIndent, "debug.push_indent")
    iHideInDebugTrace();

    iValidate
    {
    }

    iDoc(R"(    
        Increases the indentation in debugging output by one level.  Note: The
        indentation level is global across all debug streams, but separately
        tracked for each thread.
    )")

iEnd
