iBegin(file, Close, "file.close")
    iOp1(optype::ref\ <file>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <file>>(op1->type());

    }

    iDoc(R"(    
        Closes the file *op1*. Further write operations will not be possible
        (unless reopened.  Other file objects still referencing the same
        physical file will be able to continue writing.
    )")

iEnd

iBegin(file, Open, "file.open")
    iOp1(optype::ref\ <file>, trueX)
    iOp2(optype::string, trueX)
    iOp3(optype::[(enum,enum,string)], trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <file>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());
        auto ty_op3 = as<type::[(enum,enum,string)]>(op3->type());

    }

    iDoc(R"(    
        Opens a file *op1* for writing. *op2* is the path of the file. If not
        absolute, it is interpreted relative to the current directory. *op3*
        is tuple consisting of (1) the file type, either
        ~~Hilti::FileType::Text or ~~Hilti::FileType::Binary; (2) the file
        open mode, either ~~Hilti::FileMode::Create or
        ~~Hilti::FileMode::Append; and (3) a string giveing the output
        character set for writing out strings. If *op3* is not given, the
        default is ``(Hilti::FileType::Text, Hilti::FileMode::Create,
        "utf8")``.  Raises ~~IOError if there was a problem opening the file.
    )")

iEnd

iBegin(file, Write, "file.write")
    iOp1(optype::ref\ <file>, trueX)
    iOp2(optype::string | ref<bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <file>>(op1->type());
        auto ty_op2 = as<type::string | ref<bytes>>(op2->type());

    }

    iDoc(R"(    
        Writes *op1* into the file. If *op1* is a string, it will be encoded
        according to the character set given to ~~file.open. If the file was
        opened in text mode, unprintable bytes characters will be suitably
        escaped and all lines will be terminated with newlines. It is
        guaranteed that a single execution of this instruction is atomic in
        the sense that all characters will be written out in one piece even if
        other threads are performing writes to the same file concurrently.
        Multiple independent write call may however be interleaved with calls
        from other threads.
    )")

iEnd

