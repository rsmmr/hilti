
#include "filter.h"
#include "sink.h"

// These are defined in filter_*.c.
extern binpac_filter* __binpac_filter_base64_new(hlt_exception** excpt, hlt_execution_context* ctx);

static hlt_string_constant FilterName = { 34, "<should show the filter name here>" };

void __binpac_filter_init(binpac_filter* filter, const char* name, __binpac_filter_decode decode, __binpac_filter_close close)
{
    filter->name = name;
    filter->decode = decode;
    filter->close = close;
    filter->next = 0;
}

binpac_filter* binpac_filter_add(binpac_filter* head, binpac_filter* add, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! head )
        return add;

    while ( head->next )
        head = head->next;

    head->next = add;

    return head;
}

binpac_filter* binpac_filter_new(hlt_enum ftype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_filter* filter = 0;

    if ( hlt_enum_equal(ftype, Binpac_Filter_BASE64, excpt, ctx) )
        return (binpac_filter*) __binpac_filter_base64_new(excpt, ctx);

    hlt_set_exception(excpt, &binpac_exception_filterunsupported, &FilterName);
    return 0;
}

// We borrow this from sink.c
extern void binpac_dbg_print_data(binpac_sink* sink, hlt_bytes* data, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx);

hlt_bytes* binpac_filter_decode(binpac_filter* filter, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    while( filter ) {
        data = (*filter->decode)(filter, data, excpt, ctx);
        if ( *excpt )
            // Error.
            return 0;

        binpac_dbg_print_data(0, data, filter, excpt, ctx);

        filter = filter->next;
    }

    return data;
}

void binpac_filter_close(binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx)
{
    while ( filter ) {
        (*filter->close)(filter, excpt, ctx);
        filter = filter->next;
    }
}
