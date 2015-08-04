
opBegin(iterator::Equal)
    opOp1(std::make_shared<type::Iterator>())
    opOp2(std::make_shared<type::Iterator>())

    opDoc("Compares two iterators.")

    opValidate() {
        sameType(op1()->type(), op2()->type());
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(iterator::Deref)
    opOp1(std::make_shared<type::Iterator>())

    opDoc("Returns the element referenced by the iterator.")

    opValidate() {
    }

    opResult() {
        if (ast::isA<type::Bytes>(ast::checkedCast<type::Iterator>(op1()->type())->argType())){
            return std::make_shared<type::Integer>(8,false);   
        }
        else {
            return ast::checkedCast<type::Iterator>(op1()->type())->argType();
        }
    }
opEnd

opBegin(iterator::IncrPostfix)
    opOp1(std::make_shared<type::Iterator>())

    opDoc("Advances the iterator by one element, returning the previous iterator.")

    opValidate() {
    }

    opResult() {
        return op1()->type();
    }
opEnd

opBegin(iterator::IncrPrefix)
    opOp1(std::make_shared<type::Iterator>())

    opDoc("Advances the iterator by one element, returning the new iterator.")

    opValidate() {
    }

    opResult() {
        return op1()->type();
    }
opEnd

opBegin(iterator::Plus)
    opOp1(std::make_shared<type::Iterator>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Returns an iterator advanced by a given number of elements.")

    opValidate() {
    }

    opResult() {
        return op1()->type();
    }
opEnd

opBegin(iterator::PlusAssign)
    opOp1(std::make_shared<type::Iterator>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Advances the iterator by a given number of elements.")

    opValidate() {
    }

    opResult() {
        return op1()->type();
    }
opEnd

// opBegin(iterator::Plus)
//     opOp1(std::make_shared<type::Iterator>())
//     opOp2(std::make_shared<type::Integer>())
// 
//     opDoc("Returns the iterator advanced by a given number of elements.")
// 
//     opValidate() {
//         if ( ast::checkedCast<type::Integer>(op2()->type())->signed_() )
//             error(op2(), "integer must be unsigned");
//     }
// 
//     opResult() {
//         return op1()->type();
//     }
// opEnd
// 
// opBegin(iterator::PlusAssign)
//     opOp1(std::make_shared<type::Iterator>())
//     opOp2(std::make_shared<type::Integer>())
// 
//     opDoc("Advances the iterator by a given number of elements.")
// 
//     opValidate() {
//         if ( ast::checkedCast<type::Integer>(op2()->type())->signed_() )
//             error(op2(), "integer must be unsigned");
//     }
// 
//     opResult() {
//         return op1()->type();
//     }
// opEnd
