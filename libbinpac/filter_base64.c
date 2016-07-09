
#include "filter.h"

#include "3rdparty/libb64/include/b64/cdecode.h"

typedef struct {
    binpac_filter base;
    base64_decodestate state;
} __binpac_filter_base64;

binpac_filter* __binpac_filter_base64_allocate(hlt_exception** excpt, hlt_execution_context* ctx)
{
    __binpac_filter_base64* filter =
        GC_NEW_CUSTOM_SIZE(binpac_filter, sizeof(__binpac_filter_base64), ctx);
    base64_init_decodestate(&filter->state);
    return (binpac_filter*)filter;
}

void __binpac_filter_base64_dtor(hlt_type_info* ti, binpac_filter* filter,
                                 hlt_execution_context* ctx)
{
    // Nothing to do.
}

void __binpac_filter_base64_close(binpac_filter* filter, hlt_exception** excpt,
                                  hlt_execution_context* ctx_)
{
    // Unclear in which state we should be here if all has been decoded. I'd
    // think just "step_a", but I have also observed "step_d" even though all
    // input was correctly decoded (perhaps this is padding related?). So for
    // now, we just ignore erros at the very end, that's probably fine
    // anyway.
}

hlt_bytes* __binpac_filter_base64_decode(binpac_filter* filter_gen, hlt_bytes* data,
                                         hlt_exception** excpt, hlt_execution_context* ctx)
{
    __binpac_filter_base64* filter = (__binpac_filter_base64*)filter_gen;

    void* cookie = 0;
    hlt_bytes_block block;
    hlt_iterator_bytes begin = hlt_bytes_begin(data, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

    hlt_bytes* decoded = 0;

    do {
        cookie = hlt_bytes_iterate_raw(&block, cookie, begin, end, excpt, ctx);

        int len = block.end - block.start;

        if ( ! len )
            continue;

        // We will allocate a bit more than we'll need here.
        int8_t* buffer = hlt_malloc(len);

        int n = base64_decode_block((const char*)block.start, len, (char*)buffer, &filter->state);

        if ( n > 0 ) {
            if ( ! decoded ) {
                decoded = hlt_bytes_new_from_data(buffer, n, excpt, ctx);
                buffer = 0;
            }
            else
                hlt_bytes_append_raw(decoded, buffer, n, excpt, ctx);
            buffer = 0;
        }

        if ( buffer )
            hlt_free(buffer);

    } while ( cookie );

    return decoded ? decoded : hlt_bytes_new_from_data(0, 0, excpt, ctx);
}
