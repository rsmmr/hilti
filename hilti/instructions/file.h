///
/// \type File
/// 
/// \cproto hlt_file*
///

iBegin(file, New, "new")
    iTarget(optype::refFile)
    iOp1(optype::typeFile, true)

    iValidate {
    }

    iDoc(R"(
       Instantiates a new ``file`` instance, which will initially be closed
       and not associated with any actual file.
    )")

iEnd


iBegin(file, Close, "file.close")
    iOp1(optype::refFile, false)

    iValidate {
    }

    iDoc(R"(    
        Closes the file *op1*. Further write operations will not be possible
        (unless reopened.  Other file objects still referencing the same
        physical file will be able to continue writing.
    )")

iEnd

iBegin(file, Open, "file.open")
    iOp1(optype::refFile, false)
    iOp2(optype::string, true)
    iOp3(optype::tuple, true)

    iValidate {
        // auto ty_op1 = as<type::refFile>(op1->type());
        // auto ty_op2 = as<type::string>(op2->type());
        // auto ty_op3 = as<type::[(enum,enum,string)]>(op3->type());

        // TODO: Check tuple argument.
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

iBegin(file, WriteString, "file.write")
    iOp1(optype::refFile, false)
    iOp2(optype::string, true)

    iValidate {
    }

    iDoc(R"(    
        Writes *op1* into the file. The output will be encoded
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

iBegin(file, WriteBytes, "file.write")
    iOp1(optype::refFile, false)
    iOp2(optype::refBytes, true)

    iValidate {
    }

    iDoc(R"(    
        Writes *op1* into the file. If the file was
        opened in text mode, unprintable bytes characters will be suitably
        escaped and all lines will be terminated with newlines. It is
        guaranteed that a single execution of this instruction is atomic in
        the sense that all characters will be written out in one piece even if
        other threads are performing writes to the same file concurrently.
        Multiple independent write call may however be interleaved with calls
        from other threads.
    )")

iEnd

