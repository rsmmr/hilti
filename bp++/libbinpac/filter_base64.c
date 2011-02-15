
#include "filter.h"

#include <b64/cdecode.h>

typedef struct {
    binpac_filter base;
    base64_decodestate state;
} __binpac_filter_base64;

extern hlt_bytes* __binpac_filter_base64_decode(__binpac_filter_base64* filter, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* cookie = 0;
    hlt_bytes_block block;
    hlt_bytes_pos begin = hlt_bytes_begin(data, excpt, ctx);
    hlt_bytes_pos end = hlt_bytes_end(data, excpt, ctx);

    hlt_bytes* decoded = 0;

    do {
        cookie = hlt_bytes_iterate_raw(&block, cookie, begin, end, excpt, ctx);

        int len = block.end - block.start;

        if ( ! len )
            continue;

        // We will allocate a bit more than we'll need here.
        int8_t* buffer = hlt_gc_malloc_atomic(len);

        if ( ! buffer ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return 0;
        }

        int n = base64_decode_block((const char*)block.start, len, (char *)buffer, &filter->state);

        if ( n > 0 ) {
            if ( ! decoded )
                decoded = hlt_bytes_new_from_data(buffer, n, excpt, ctx);
            else
                hlt_bytes_append_raw(decoded, buffer, n, excpt, ctx);
        }

    } while ( cookie );

    return decoded ? decoded : hlt_bytes_new_from_data((int8_t*)"", 0, excpt, ctx);
}

extern void __binpac_filter_base64_close(__binpac_filter_base64* filter, hlt_exception** excpt, hlt_execution_context* ctx_)
{
#if 0
    if ( filter->state.step != step_a )
        // Not all input has been decoded.
        hlt_set_exception(excpt, &binpac_exception_filtererror, 0);
#endif
    // TODO: Unclear in which state we should be here if all has been
    // decoded. I'd think just "step_a", but I have also observed "step_d"
    // even though all input was correctly decoded (perhaps this is padding
    // related?). So for now, we just ignore erros at the very end, that's
    // probably fine anyway.
}

extern binpac_filter* __binpac_filter_base64_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    __binpac_filter_base64* filter = hlt_gc_malloc_non_atomic(sizeof(__binpac_filter_base64));

    if ( ! filter ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    __binpac_filter_init((binpac_filter*)filter,
                         "BASE64",
                         (__binpac_filter_decode) __binpac_filter_base64_decode,
                         (__binpac_filter_close)  __binpac_filter_base64_close);

    base64_init_decodestate(&filter->state);

    return (binpac_filter*)filter;
}

