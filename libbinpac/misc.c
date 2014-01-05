
#include "misc.h"

hlt_string binpac_fmt_string(hlt_string fmt, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hilti_fmt(fmt, type, tuple, excpt, ctx);
}

hlt_bytes* binpac_fmt_bytes(hlt_bytes* fmt, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // TODO: We hardcode the character set here for now. Pass as additional
    // parameter?
    hlt_enum charset = Hilti_Charset_ASCII;

    hlt_string sfmt = hlt_string_decode(fmt, charset, excpt, ctx);
    hlt_string sresult = hilti_fmt(sfmt, type, tuple, excpt, ctx);
    hlt_bytes* bresult = hlt_string_encode(sresult, charset, excpt, ctx);

    GC_DTOR(sfmt, hlt_string);
    GC_DTOR(sresult, hlt_string);

    return bresult;
}
