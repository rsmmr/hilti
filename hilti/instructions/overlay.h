
static shared_ptr<type::overlay::Field> _overlayField(const Instruction* i, shared_ptr<Expression> op, shared_ptr<Expression> field)
{
    auto otype = ast::as<type::Overlay>(op->type());
    assert(otype);

    auto cexpr = ast::as<expression::Constant>(field);

    if ( ! cexpr ) {
        i->error(field, "overlay field must be a constant");
        return nullptr;
    }

    auto cval = ast::as<constant::String>(cexpr->constant());

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
    iOp1(optype::overlay, false)
    iOp2(optype::iterBytes, true)

    iValidate {
    }

    iDoc(R"(    
        Associates the overlay in *op1* with a bytes object, aligning the
        overlays offset 0 with the position specified by *op2*. The *attach*
        operation must be performed before any other ``overlay`` instruction
        is used on *op1*.
    )")

iEnd

iBegin(overlay, Get, "overlay.get")
    iTarget(optype::any)
    iOp1(optype::overlay, false)
    iOp2(optype::string, true)

    iValidate {
        if ( ! isConstant(op2) )
            return;

        auto f = _overlayField(this, op1, op2);

        if ( ! f )
            return;

        canCoerceTo(f->type(), target);
    }

    iDoc(R"(    
        Unpacks the field named *op2* from the bytes object attached to the
        overlay *op1*. The field name must be a constant, and the type of the
        target must match the field's type.  The instruction throws an
        OverlayNotAttached exception if ``overlay.attach`` has not been
        executed for *op1* yet.
    )")

iEnd

