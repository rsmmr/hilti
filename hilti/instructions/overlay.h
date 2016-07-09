
static shared_ptr<type::overlay::Field> _overlayField(const Instruction* i,
                                                      shared_ptr<Type> overlay,
                                                      shared_ptr<Expression> field)
{
    auto otype = ast::rtti::checkedCast<type::Overlay>(overlay);
    ;
    auto cexpr = ast::rtti::tryCast<expression::Constant>(field);

    if ( ! cexpr ) {
        i->error(field, "overlay field must be a constant");
        return nullptr;
    }

    auto cval = ast::rtti::tryCast<constant::String>(cexpr->constant());

    if ( ! cval ) {
        i->error(field, "overlay field must be a constant string");
        return nullptr;
    }

    auto name = cval->value();
    auto f = otype->field(name);

    if ( ! f )
        i->error(field, util::fmt("unknown overlay field '%s'", name.c_str()));

    return f;
}

iBegin(overlay, Attach, "overlay.attach")
    iOp1(optype::overlay, false);
    iOp2(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(    
        Associates the overlay in *op1* with a bytes object, aligning the
        overlays offset 0 with the position specified by *op2*. The *attach*
        operation must be performed before any other ``overlay`` instruction
        is used on *op1*.
    )")

iEnd

iBegin(overlay, Get, "overlay.get")
    iTarget(optype::any);
    iOp1(optype::overlay, false);
    iOp2(optype::string, true);
    iOp3(optype::optional(optype::refBytes), true);

    iValidate
    {
        auto otype = ast::rtti::tryCast<type::Overlay>(op1->type());

        if ( ! isConstant(op2) )
            return;

        auto f = _overlayField(this, op1->type(), op2);

        if ( ! f )
            return;

        if ( op3 && otype->numDependencies() )
            error(op1, "cannot have dependent fields when working with detached overlay");

        canCoerceTo(f->type(), target);
    }

    iDoc(R"(    
        Unpacks the field named *op2* from the bytes object attached to the
        overlay *op1*, if *op3* is not given. If *op3* is given, uses that bytes object
        instead; the overlay can be detached in that case. If *op3* is given, the overlay
        type must not have any fields at variable offsets.
        The field name must be a constant, and the type of the
        target must match the field's type.  The instruction throws an
        OverlayNotAttached exception if ``overlay.attach`` has not been
        executed for *op1* yet.
    )")
iEnd

iBegin(overlay, GetStatic, "overlay.get")
    iTarget(optype::any);
    iOp1(optype::typeOverlay, true);
    iOp2(optype::string, true);
    iOp3(optype::refBytes, true);

    iValidate
    {
        auto otype = ast::rtti::checkedCast<type::Overlay>(typedType(op1));

        if ( ! isConstant(op2) )
            return;

        auto f = _overlayField(this, otype, op2);

        if ( ! f )
            return;

        if ( otype->numDependencies() )
            error(op1, "cannot have dependent fields when working with detached overlay");

        canCoerceTo(f->type(), target);
    }

    iDoc(R"(    
        Unpacks the field named *op2* of overlay type *op1* from the bytes object *op3*.
        This is a stateless variant of the *get* instructions that does not need an actual
        overlay instance. However, it requires that the overlay
        type must not have any fields at variable offsets.
    )")

iEnd
