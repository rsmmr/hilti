
// This handles both gzip and zlib/deflate decompresssion.

#include <zlib.h>

#include "filter.h"

typedef struct {
    binpac_filter base;
	z_stream* zip;
} __binpac_filter_zlib;

void __binpac_filter_zlib_close(binpac_filter* filter_gen, hlt_exception** excpt, hlt_execution_context* ctx_)
{
    __binpac_filter_zlib* filter = (__binpac_filter_zlib *)filter_gen;

    if ( ! filter->zip )
        return;

    inflateEnd(filter->zip);
    hlt_free(filter->zip);
    filter->zip = 0;
}

binpac_filter* __binpac_filter_zlib_allocate(hlt_exception** excpt, hlt_execution_context* ctx)
{
    __binpac_filter_zlib* filter = GC_NEW_CUSTOM_SIZE(binpac_filter, sizeof(__binpac_filter_zlib));
    filter->zip = hlt_malloc(sizeof(z_stream));
	filter->zip->zalloc = 0;
	filter->zip->zfree = 0;
	filter->zip->opaque = 0;
	filter->zip->next_out = 0;
	filter->zip->avail_out = 0;
	filter->zip->next_in = 0;
	filter->zip->avail_in = 0;

	// "15" here means maximum compression.  "32" is a gross overload hack
	// that means "check it for whether it's a gzip file". Sheesh.
	int zip_status = inflateInit2(filter->zip, 15 + 32);

	if ( zip_status != Z_OK ) {
        __binpac_filter_zlib_close((binpac_filter*)filter, excpt, ctx);
        hlt_string fname = hlt_string_from_asciiz("inflateInit2 failed", excpt, ctx);
        hlt_set_exception(excpt, &binpac_exception_filtererror, fname);
        return 0;
    }

    return (binpac_filter*)filter;
}

void __binpac_filter_zlib_dtor(hlt_type_info* ti, binpac_filter* filter)
{
    __binpac_filter_zlib_close(filter, 0, 0);
}

hlt_bytes* __binpac_filter_zlib_decode(binpac_filter* filter_gen, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __binpac_filter_zlib* filter = (__binpac_filter_zlib *)filter_gen;

    if ( ! filter->zip ) {
        hlt_string fname = hlt_string_from_asciiz("more inflate data after close", excpt, ctx);
        hlt_set_exception(excpt, &binpac_exception_filtererror, fname);
        return 0;
    }

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

        filter->zip->next_in = (Bytef*)block.start;
        filter->zip->avail_in = len;

        do {
            Bytef buf[4096];
            filter->zip->next_out = buf;
            filter->zip->avail_out = sizeof(buf);

            int zip_status = inflate(filter->zip, Z_SYNC_FLUSH);

            if ( zip_status != Z_STREAM_END &&
                 zip_status != Z_OK &&
                 zip_status != Z_BUF_ERROR ) {
                __binpac_filter_zlib_close((binpac_filter*)filter, excpt, ctx);
                hlt_string fname = hlt_string_from_asciiz("inflate failed", excpt, ctx);
                hlt_set_exception(excpt, &binpac_exception_filtererror, fname);
                return 0;
            }

            int have = sizeof(buf) - filter->zip->avail_out;

            if ( have ) {
                if ( ! decoded )
                    decoded = hlt_bytes_new_from_data(0, 0, excpt, ctx);
                else
                    hlt_bytes_append_raw_copy(decoded, (int8_t*)buf, have, excpt, ctx);
            }

            if ( zip_status == Z_STREAM_END ) {
                __binpac_filter_zlib_close((binpac_filter*)filter, excpt, ctx);
                break;
			}

		} while ( filter->zip->avail_out == 0 );

    } while ( cookie );

    GC_DTOR(begin, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return decoded ? decoded : hlt_bytes_new_from_data(0, 0, excpt, ctx);
}
