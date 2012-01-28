iBegin(debug, Assert, "debug.assert")
    iOp1(optype::bool, trueX)
    iOp2(optype::[string], trueX)

    iValidate {
        auto ty_op1 = as<type::bool>(op1->type());
        auto ty_op2 = as<type::[string]>(op2->type());

    }

    iDoc(R"(    
        If compiled in debug mode, verifies that *op1* is true. If not, throws
        an ~~AssertionError exception with *op2* as its value if given.
    )")

iEnd

iBegin(debug, InternalError, "debug.internal_error")
    iOp1(optype::string, trueX)

    iValidate {
        auto ty_op1 = as<type::string>(op1->type());

    }

    iDoc(R"(    
        Throws an InternalError exception, setting *op1* as the its argument.
        Note: This is just a convenience instruction; one can also raise the
        exception directly.
    )")

iEnd

iBegin(debug, Msg, "debug.msg")
    iOp1(optype::string, trueX)
    iOp2(optype::string, trueX)
    iOp3(optype::tuple, trueX)

    iValidate {
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());
        auto ty_op3 = as<type::tuple>(op3->type());

    }

    iDoc(R"(    
        If compiled in debug mode, prints a debug message to stderr. The
        message is only printed of the debug stream *op1* has been activated.
        *op2* is printf-style format string and *op3* the correponding
        arguments.
    )")

iEnd

iBegin(debug, InternalError, "debug.pop_indent")

    iValidate {

    }

    iDoc(R"(    
        Decreases the indentation in debugging output by one level.  Note: The
        indentation level is global across all debug streams, but separately
        tracked for each thread.
    )")

iEnd

iBegin(debug, PrintFrame, "debug.print_frame")

    iValidate {

    }

    iDoc(R"(    
        An internal debugging instruction that prints out pointers making up
        the current stack frame.
    )")

iEnd

iBegin(debug, InternalError, "debug.push_indent")

    iValidate {

    }

    iDoc(R"(    
        Increases the indentation in debugging output by one level.  Note: The
        indentation level is global across all debug streams, but separately
        tracked for each thread.
    )")

iEnd

