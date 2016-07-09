/// \type Enums
///
/// The ``enum`` data type represents a selection of unique values,
/// identified by labels. Enums must be defined in global space:
///
///     enum Foo { Red, Green, Blue }
///
/// The individual labels can then be used as global identifiers. In addition
/// to the given labels, each ``enum`` type also implicitly defines one
/// additional label called ''Undef''.
///
/// If not explictly initialized, enums are set to ``Undef`` initially.
///
/// TODO: Extend descriptions with the new semantics regarding storing
/// undefined values.
///
/// \default Undef
///
/// \cproto hlt_enum

#include "define-instruction.h"

iBegin(enum_, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::enum_, true);
    iOp2(optype::enum_, true);

    iValidate
    {
        auto ty_op1 = ast::rtti::checkedCast<type::Enum>(op1->type());
        auto ty_op2 = ast::rtti::checkedCast<type::Enum>(op2->type());
        equalTypes(ty_op1, ty_op2);
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd

iBegin(enum_, FromInt, "enum.from_int")
    iTarget(optype::enum_);
    iOp1(optype::integer, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the integer *op2* into an enum of the target's *type*. If the
        integer corresponds to a valid label (as set by explicit
        initialization), the results corresponds to that label. If integer
        does not correspond to any lable, the result will be of value
        ``Undef`` (yet internally, the integer value is retained.)
    )")

iEnd

iBegin(enum_, ToInt, "enum.to_int")
    iTarget(optype::integer);
    iOp1(optype::enum_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the enum *op1* into an integer value.enum. If the enum is undefined, it will return -1.
    )")

iEnd
