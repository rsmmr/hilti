
#include "misc.h"

hlt_string binpac_fmt_string(hlt_string fmt, const hlt_type_info* type, void* tuple, hlt_exception** excpt, hlt_execution_context* ctx) // &noref
{
    return hilti_fmt(fmt, type, tuple, excpt, ctx);
}

hlt_bytes* binpac_fmt_bytes(hlt_bytes* fmt, const hlt_type_info* type, void* tuple, hlt_exception** excpt, hlt_execution_context* ctx) // &noref
{
    // TODO: We hardcode the character set here for now. Pass as additional
    // parameter?
    hlt_enum charset = Hilti_Charset_ASCII;

    hlt_string sfmt = hlt_string_decode(fmt, charset, excpt, ctx);
    hlt_string sresult = hilti_fmt(sfmt, type, tuple, excpt, ctx);
    hlt_bytes* bresult = hlt_string_encode(sresult, charset, excpt, ctx);

    return bresult;
}

hlt_time binpac_mktime(int64_t y, int64_t m, int64_t d, int64_t H, int64_t M, int64_t S, hlt_exception** excpt, hlt_execution_context* ctx)
{
    struct tm t;
    t.tm_sec = S;
    t.tm_min = M;
    t.tm_hour = H;
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    t.tm_isdst = -1;

    time_t teatime = mktime(&t);

    if ( teatime >= 0 )
        return hlt_time_value(teatime, 0);

    hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
    return 0;
}

