
/// \type String
///
/// Bla bla.
///
/// \constant "abc"
///
/// \cproto hlt_string

iBegin(string, Concat, "string.concat")
    iTarget(type::String)
    iOp1(type::String, true)
    iOp2(type::String, true)

    iValidate {
    }

    iDoc("Documentation")
iEnd


