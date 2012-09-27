
opBegin(unit::Attribute)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())

    opDoc("Access a unit field.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        auto i = unit->item(attr->id());
        assert(i);
        assert(i->type());

        return i->type();
    }
opEnd

opBegin(unit::AttributeAssign)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())
    opOp3(std::make_shared<type::Any>())

    opDoc("Assign a value to a unit field.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        auto i = unit->item(attr->id());
        assert(i);
        assert(i->type());

        return i->type();
    }
opEnd
