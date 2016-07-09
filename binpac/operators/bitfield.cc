
using namespace binpac;

opBegin(bitfield::Attribute)
    opOp1(std::make_shared<type::Bitfield>());
    opOp2(std::make_shared<type::MemberAttribute>());

    opDoc("Access a bitfield element.");

    opValidate()
    {
        auto btype = ast::checkedCast<type::Bitfield>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! btype->bits(attr->id()) )
            error(op2(), "unknown bitfield element");
    }

    opResult()
    {
        auto btype = ast::checkedCast<type::Bitfield>(op1()->type());
        return std::make_shared<type::Integer>(btype->width(), false);
    }
opEnd
