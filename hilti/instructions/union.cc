///
/// \type Structures
///
/// The ``struct`` data type groups a set of heterogenous, named fields. Each
/// instance tracks which fields have already been set. Fields may optionally
/// provide default values that are returned when it's read but hasn't been
/// set yet.
///
///

#include "define-instruction.h"

#include "../module.h"
#include "union.h"

static shared_ptr<type::union_::Field> _unionField(const Instruction* i,
                                                   shared_ptr<type::Union> stype,
                                                   shared_ptr<Expression> field)
{
    auto cexpr = ast::rtti::tryCast<expression::Constant>(field);

    if ( ! cexpr ) {
        i->error(field, "union field must be a constant");
        return nullptr;
    }

    auto cval = ast::rtti::tryCast<constant::String>(cexpr->constant());

    if ( ! cval ) {
        i->error(field, "union field must be a constant string");
        return nullptr;
    }

    auto name = cval->value();

    for ( auto f : stype->fields() ) {
        if ( f->id()->name() == name )
            return f;
    }

    i->error(field, util::fmt("unknown union field '%s'", name.c_str()));
    return nullptr;
}

static shared_ptr<type::union_::Field> _unionField(const Instruction* i, shared_ptr<Expression> op,
                                                   shared_ptr<Expression> field)
{
    auto stype = ast::rtti::tryCast<type::Union>(op->type());

    if ( ! stype ) {
        i->error(op, "not a union type");
        return nullptr;
    }

    return _unionField(i, stype, field);
}

static shared_ptr<type::union_::Field> _unionField(const Instruction* i,
                                                   shared_ptr<type::Union> stype,
                                                   shared_ptr<Type> field_type)
{
    auto fields = stype->fields(field_type);

    if ( fields.size() == 0 ) {
        i->error(field_type, "no union field of that type");
        return nullptr;
    }

    if ( fields.size() > 1 ) {
        i->error(field_type, "more than oen union field of that type");
        return nullptr;
    }

    return fields.front();
}

static shared_ptr<type::union_::Field> _unionField(const Instruction* i, shared_ptr<Expression> op,
                                                   shared_ptr<Type> field_type)
{
    auto stype = ast::rtti::tryCast<type::Union>(op->type());
    return _unionField(i, stype, field_type);
}

iBeginCC(union_)
    iValidateCC(InitField)
    {
        if ( ! isConstant(op2) )
            return;

        auto utype = ast::rtti::tryCast<type::Union>(typedType(op1));
        if ( ! utype ) {
            error(op1, "not a union type");
            return;
        }

        auto f = _unionField(this, utype, op2);
        if ( ! f )
            return;

        canCoerceTo(op3, f->type());
    }

    iDocCC(InitField, R"(
        Returns a union of type *op1* with the field named *op2* set to
        value *op3*. The type of the *op3* must match the type of the field. All other fields
        will automatically become unset.
    )")
iEndCC

iBeginCC(union_)
    iValidateCC(InitType)
    {
        auto utype = ast::rtti::tryCast<type::Union>(typedType(op1));
        if ( ! utype ) {
            error(op1, "not a union type");
            return;
        }

        auto f = _unionField(this, utype, op2->type());
        if ( ! f )
            return;
    }

    iDocCC(InitType, R"(
        Returns a union of type *op1* with the field of the same type as *op2* to *op2*.
        There must be exactly one field of that type. All other fields
        will automatically become unset.
    )")
iEndCC

iBeginCC(union_)
    iValidateCC(GetField)
    {
        if ( ! isConstant(op2) )
            return;

        auto f = _unionField(this, op1, op2);

        if ( ! f )
            return;

        canCoerceTo(f->type(), target);
    }

    iDocCC(GetField, R"(
        Returns the field named *op2* in the union referenced by *op1*. The
        field name must be a constant, and the type of the target must match
        the field's type. If a field is requested that is not currently set, an
        ``UndefinedValue`` exception is raised.
    )")
iEndCC

iBeginCC(union_)
    iValidateCC(GetType)
    {
        auto f = _unionField(this, op1, target->type());

        if ( ! f )
            return;
    }

    iDocCC(GetType, R"(
        Returns the value of the field in the union that has the same type as *target*.
        There must be exactl one field of that type. If a field is requested that is
        not currently set, an ``UndefinedValue`` exception is raised.
    )")
iEndCC

iBeginCC(union_)
    iValidateCC(IsSetField)
    {
        if ( ! isConstant(op2) )
            return;

        _unionField(this, op1, op2);
    }

    iDocCC(IsSetField, R"(
        Returns *True* if the field named *op2* is current set to  a value, and
        *False otherwise. If the *union.is_set* returns *True*, a subsequent call
        to ~~Get for that field will not raise an exception.
    )")
iEndCC

iBeginCC(union_)
    iValidateCC(IsSetType)
    {
        _unionField(this, op1, typedType(op2));
    }

    iDocCC(IsSetType, R"(
        Returns *True* if the field that has type *op2* is currently set to a value, and
        *False otherwise. There must be exactly one field of type *op2*.
    )")
iEndCC
