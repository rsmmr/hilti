iBegin(string, Cmp, "string.cmp")
    iTarget(optype::bool)
    iOp1(optype::string, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Compares *op1* with *op2* and returns True if their characters match.
        Returns False otherwise.
    )")

iEnd

iBegin(string, Concat, "string.concat")
    iTarget(optype::string)
    iOp1(optype::string, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::string>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Concatenates *op1* with *op2* and returns the result.
    )")

iEnd

iBegin(string, Decode, "string.decode")
    iTarget(optype::string)
    iOp1(optype::ref\ <bytes>, trueX)
    iOp2(optype::"ascii"|"utf8", trueX)

    iValidate {
        auto ty_target = as<type::string>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());
        auto ty_op2 = as<type::"ascii"|"utf8">(op2->type());

    }

    iDoc(R"(    
        Converts *bytes op1* into a string, assuming characters are encoded in
        character set *op2*. Supported character sets are currently:
        ``ascii``, ``utf8``.  If the string cannot be decoded with the given
        character set, the instruction throws an ~~DecodingError exception. If
        the character set given is not known, the instruction throws a
        ~~ValueError exception.  Todo: We need to figure out how to support
        more character sets.
    )")

iEnd

iBegin(string, Encode, "string.encode")
    iTarget(optype::ref\ <bytes>)
    iOp1(optype::string, trueX)
    iOp2(optype::"ascii"|"utf8", trueX)

    iValidate {
        auto ty_target = as<type::ref\ <bytes>>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::"ascii"|"utf8">(op2->type());

    }

    iDoc(R"(    
        Converts *op1* into bytes, encoding characters using the character set
        *op2*. Supported character sets are currently: ``ascii``, ``utf8``.
        If the any characters cannot be encoded with the given character set,
        they will be replaced with place-holders. If the character set given
        is not known, the instruction throws a ~~ValueError exception.  Todo:
        We need to figure out how to support more character sets.
    )")

iEnd

iBegin(string, Find, "string.find")
    iTarget(optype::int\ <64>)
    iOp1(optype::string, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Searches *op2* in *op1*, returning the start index if it find it.
        Returns -1 if it does not find *op2* in *op1*.
    )")

iEnd

iBegin(string, Length, "string.length")
    iTarget(optype::int\ <64>)
    iOp1(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::string>(op1->type());

    }

    iDoc(R"(    
        Returns the number of characters in the string *op1*.
    )")

iEnd

iBegin(string, Lt, "string.lt")
    iTarget(optype::bool)
    iOp1(optype::string, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Compares *op1* with *op2* and returns True if *op1* is
        lexicographically smaller than *op2*. Returns False otherwise.
    )")

iEnd

iBegin(string, Substr, "string.substr")
    iTarget(optype::string)
    iOp1(optype::string, trueX)
    iOp2(optype::int\ <64>, trueX)
    iOp3(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::string>(target->type());
        auto ty_op1 = as<type::string>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());
        auto ty_op3 = as<type::int\ <64>>(op3->type());

    }

    iDoc(R"(    
        Extracts the substring of length *op3* from *op1* that starts at
        position *op2*.
    )")

iEnd

