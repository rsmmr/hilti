///
/// \type Structures
///
/// The ``struct`` data type groups a set of heterogenous, named fields. Each
/// instance tracks which fields have already been set. Fields may optionally
/// provide default values that are returned when it's read but hasn't been
/// set yet.
///
///

static shared_ptr<type::struct_::Field> _structField(const Instruction* i,
                                                     shared_ptr<Expression> op,
                                                     shared_ptr<Expression> field)
{
    auto stype = ast::rtti::tryCast<type::Struct>(i->referencedType(op));
    assert(stype);

    auto cexpr = ast::rtti::tryCast<expression::Constant>(field);

    if ( ! cexpr ) {
        i->error(field, "struct field must be a constant");
        return nullptr;
    }

    auto cval = ast::rtti::tryCast<constant::String>(cexpr->constant());

    if ( ! cval ) {
        i->error(field, "struct field must be a constant string");
        return nullptr;
    }

    auto name = cval->value();

    for ( auto f : stype->fields() ) {
        if ( f->id()->name() == name )
            return f;
    }

    i->error(field, util::fmt("unknown struct field '%s'", name.c_str()));
    return nullptr;
}


iBegin(struct_::New, "new")
    iTarget(optype::refStruct);
    iOp1(optype::typeStruct, true);

    iValidate {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new object of the given ``struct`` type.
    )");
iEnd

iBegin(struct_::Get, "struct.get")
    iTarget(optype::any);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);

    iValidate {
        if ( ! isConstant(op2) )
            return;

        auto f = _structField(this, op1, op2);

        if ( ! f )
            return;

        canCoerceTo(f->type(), target);
    }

    iDoc(R"(
        Returns the field named *op2* in the struct referenced by *op1*. The
        field name must be a constant, and the type of the target must match
        the field's type. If a field is requested that has not been set, its
        default value is returned if it has any defined. If it has not, an
        ``UndefinedValue`` exception is raised.
    )");
iEnd

iBegin(struct_::GetDefault, "struct.get_default")
    iTarget(optype::any);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);
    iOp3(optype::any, false);

    iValidate {
        if ( ! isConstant(op2) )
            return;

        auto f = _structField(this, op1, op2);
        if ( ! f )
            return;

        canCoerceTo(f->type(), target);
        canCoerceTo(op3, target);
    }

    iDoc(R"(
        Returns the field named *op2* in the struct referenced by *op1*, or a
        default value *op3* if not set (if the field has a default itself, that
        however has priority). The field name must be a constant, and the type
        of the target must match the field's type, as must that of the default
        value.
    )");
iEnd

iBegin(struct_::IsSet, "struct.is_set")
    iTarget(optype::boolean);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);

    iValidate {
        if ( ! isConstant(op2) )
            return;

        _structField(this, op1, op2);
    }

    iDoc(R"(
        Returns *True* if the field named *op2* has been set to a value, and
        *False otherwise. If the instruction returns *True*, a subsequent call
        to ~~Get will not raise an exception.
    )");
iEnd

iBegin(struct_::Set, "struct.set")
    iOp1(optype::refStruct, false);
    iOp2(optype::string, true);
    iOp3(optype::any, false);

    iValidate {
        if ( ! isConstant(op2) )
            return;

        auto f = _structField(this, op1, op2);
        if ( ! f )
            return;

        canCoerceTo(op3, f->type());
    }

    iDoc(R"(
        Sets the field named *op2* in the struct referenced by *op1* to the
        value *op3*. The type of the *op3* must match the type of the field.
    )");
iEnd

iBegin(struct_::Unset, "struct.unset")
    iOp1(optype::refStruct, false);
    iOp2(optype::string, true);

    iValidate {
        if ( ! isConstant(op2) )
            return;

        _structField(this, op1, op2);
    }

    iDoc(R"(
        Unsets the field named *op2* in the struct referenced by *op1*. An
        unset field appears just as if it had never been assigned an value;
        in particular, it will be reset to its default value if has been one
        assigned.
    )");
iEnd
