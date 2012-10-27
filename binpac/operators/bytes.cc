
opBegin(iterBytes::Deref)
    opOp1(std::make_shared<type::iterator::Bytes>())

    opDoc("Returns the character referenced by the iterator.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(8, false);
    }
opEnd

opBegin(iterBytes::IncrPostfix)
    opOp1(std::make_shared<type::iterator::Bytes>())

    opDoc("Advances the iterator by one byte")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd
