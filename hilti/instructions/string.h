
/// \type String
///
/// Bla bla.
///
/// \constant "abc"
///
/// \cproto hlt_string

iBegin(string, Cat, "string.cat")
    iTarget(type::String)
    iOp1(type::String, true)
    iOp2(type::String, true)

    iValidate {
    }

    iDoc("Documentation")
iEnd


