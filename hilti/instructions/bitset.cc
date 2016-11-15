/// \type Bitsets
///
/// The ``bitset`` data type groups a set of individual bits together. In each
/// instance, any of the bits is either set or not set. The individual bits are
/// identified by labels, which are declared in the bitsets type declaration::
///
///     bitset MyBits { BitA, BitB, BitC, BitD }
///
/// The individual labels can be accessed via their namespace, e.g.,
/// ``MyBits::BitC``, and used with the ``bitset.*`` instructions::
///
/// # Initialized to all bits cleared.
/// local mybits: MyBits
///
/// # Set a bit in the bit set.
/// mybits = bitset.set mybits MyBits::BitB
///
/// Normally, bits are numbered in the order they are given within the
/// ``bitset`` type definition. In the example above, ``BitA`` is the least
/// signficiant bit, ``BitB`` the next, etc. One can however also assign
/// numbers to bits to enforce a particular mapping from labels to bit positions::
///
/// bitset MyBits { BitA=2, BitB=3, BitC=5, BitD=7 }
///
/// Such specific mappings are relevant when (1) printing their numerical
/// value (see below); and (2) accessing an instance from C, as then one needs
/// to know what bit to test.
///
/// If a ``bitmap`` instance is not explicitly initialized, all bits are
/// initially cleared.
///
/// A ``bitmap`` can be printed as either a string (which will display the
/// labels of all set bits), or as an integer (which will be the numerical
/// value corresponding to all bits sets according to their positions).
///
/// Note: For efficiency reasons, HILTI for supports only up to 64 bits per
/// type; and one can only assign positions from 0 to 63.
///
/// \default All bits unset.
///
/// \cproto hlt_bitset

iBegin(bitset::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::bitset, true);
    iOp2(optype::bitset, true);

    iValidate
    {
        auto ty_op1 = ast::rtti::checkedCast<type::Bitset>(op1->type());
        auto ty_op2 = ast::rtti::checkedCast<type::Bitset>(op2->type());

        if ( ty_op1 != ty_op2 )
            error(op2, "operands must be of the same bitset type");
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd

iBegin(bitset::Clear, "bitset.clear")
    iTarget(optype::bitset);
    iOp1(optype::bitset, true);
    iOp2(optype::bitset, true);

    iValidate
    {
        auto ty_target = ast::rtti::checkedCast<type::Bitset>(target->type());
        auto ty_op1 = ast::rtti::checkedCast<type::Bitset>(op1->type());
        auto ty_op2 = ast::rtti::checkedCast<type::Bitset>(op2->type());

        if ( ty_op1 != ty_op2 )
            error(op2, "operands must be of the same bitset type");

        if ( ty_op1 != ty_target )
            error(target, "target must be of same bitset type as operands");
    }

    iDoc(R"(    
        Clears the bits set in *op2* from those set in *op1* and returns the
        result. Both operands must be of the *same* ``bitset`` type.
    )")

iEnd

iBegin(bitset::Has, "bitset.has")
    iTarget(optype::boolean);
    iOp1(optype::bitset, true);
    iOp2(optype::bitset, true);

    iValidate
    {
        auto ty_op1 = ast::rtti::checkedCast<type::Bitset>(op1->type());
        auto ty_op2 = ast::rtti::checkedCast<type::Bitset>(op2->type());

        if ( ty_op1 != ty_op2 )
            error(op2, "operands must be of the same bitset type");
    }

    iDoc(R"(    
        Returns True if all bits in *op2* are set in *op1*. Both operands must
        be of the *same* ``bitset`` type.
    )")

iEnd

iBegin(bitset::Set, "bitset.set")
    iTarget(optype::bitset);
    iOp1(optype::bitset, true);
    iOp2(optype::bitset, true);

    iValidate
    {
        auto ty_target = ast::rtti::checkedCast<type::Bitset>(target->type());
        auto ty_op1 = ast::rtti::checkedCast<type::Bitset>(op1->type());
        auto ty_op2 = ast::rtti::checkedCast<type::Bitset>(op2->type());

        if ( ty_op1 != ty_op2 )
            error(op2, "operands must be of the same bitset type");

        if ( ty_op1 != ty_target )
            error(target, "target must be of same bitset type as operands");
    }

    iDoc(R"(    
        Adds the bits set in *op2* to those set in *op1* and returns the
        result. Both operands must be of the *same* ``bitset`` type.
    )")

iEnd
