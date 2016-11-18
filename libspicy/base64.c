
#include "base64.h"

#include "3rdparty/libb64/include/b64/cdecode.h"
#include "3rdparty/libb64/include/b64/cencode.h"

hlt_bytes* spicy_base64_encode(hlt_bytes* b, hlt_exception** excpt,
                               hlt_execution_context* ctx) // &noref
{
    hlt_bytes_block block;
    hlt_iterator_bytes start = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    hlt_bytes* result = hlt_bytes_new(excpt, ctx);

    void* cookie = 0;

    base64_encodestate state;
    base64_init_encodestate(&state);

    while ( 1 ) {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);

        int len_in = block.end - block.start;
        int8_t out[len_in * 2];

        int len_out = base64_encode_block((const char*)block.start, len_in, (char*)out, &state);
        hlt_bytes_append_raw_copy(result, out, len_out, excpt, ctx);

        if ( ! cookie ) {
            len_out = base64_encode_blockend((char*)out, &state);
            // blockend always adds a trailing newline that we don't want.
            hlt_bytes_append_raw_copy(result, out, len_out - 1, excpt, ctx);
            break;
        }
    }

    return result;
}

hlt_bytes* spicy_base64_decode(hlt_bytes* b, hlt_exception** excpt,
                               hlt_execution_context* ctx) // &noref
{
    hlt_bytes_block block;
    hlt_iterator_bytes start = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    hlt_bytes* result = hlt_bytes_new(excpt, ctx);

    void* cookie = 0;

    base64_decodestate state;
    base64_init_decodestate(&state);

    while ( 1 ) {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);

        int len_in = block.end - block.start;
        int8_t out[len_in * 2];

        int len_out = base64_decode_block((const char*)block.start, len_in, (char*)out, &state);
        hlt_bytes_append_raw_copy(result, out, len_out, excpt, ctx);

        if ( ! cookie )
            break;
    }

    return result;
}
