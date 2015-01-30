
#include "3rdparty/sha2/sha2.h"
#include "3rdparty/rc4/rc4.h"

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

static void _sha256_update(sha256_ctx* sctx, hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_block block;
    hlt_iterator_bytes start = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    void* cookie = 0;

    while ( 1 ) {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);
        sha256_update(sctx, (const unsigned char*)block.start, block.end - block.start);

        if ( ! cookie )
            break;
        }
}

static char base64[] = "0123456789abcdef";

hlt_bytes* binpac_sha256(hlt_bytes* data, hlt_bytes* seed, uint64_t len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    sha256_ctx sctx;
    sha256_init(&sctx);

    _sha256_update(&sctx, seed, excpt, ctx);
    _sha256_update(&sctx, data, excpt, ctx);

    unsigned char digest[SHA256_DIGEST_SIZE];
    sha256_final(&sctx, (unsigned char *)digest);

    int in_len = sizeof(digest);
    int out_len = 2 * in_len;

    unsigned char ascii[out_len];
    unsigned char *in = digest;
    unsigned char *out = ascii;

    while ( in_len-- ) {
        *out++ = base64[(*in) >> 4];
        *out++ = base64[(*in++) & 0x0f];
    }

    return hlt_bytes_new_from_data_copy((int8_t*)ascii, (len ? len : out_len), excpt, ctx);
}

hlt_addr binpac_anonymize(hlt_addr addr, uint64_t seed, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_addr anon;

    rc4_ctx rctx;
    rc4_ks(&rctx, (const uint8_t *)&seed, sizeof(seed));

    if ( hlt_addr_is_v6(addr, excpt, ctx) )
        rc4_encrypt(&rctx, (const uint8_t *)&addr, (uint8_t *)&anon, sizeof(addr));

    else {
        struct in_addr in = hlt_addr_to_in4(addr, excpt, ctx);
        struct in_addr out;
        rc4_encrypt(&rctx, (const uint8_t *)&in, (uint8_t *)&out, sizeof(in));
        anon = hlt_addr_from_in4(out, excpt, ctx);
    }

    return anon;
}
