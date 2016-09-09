
#include "filter.h"
#include "exceptions.h"
#include "sink.h"

#include <autogen/binpachilti-hlt.h>

__HLT_RTTI_GC_TYPE(binpac_filter, HLT_TYPE_BINPAC_FILTER);

#include "filter_base64.c"
#include "filter_zlib.c"

// FIXME: Can't use the enums here directly; better way?
static struct __binpac_filter_definition filters[] = {
    {{0, 1},
     "BASE64",
     __binpac_filter_base64_allocate,
     __binpac_filter_base64_dtor,
     __binpac_filter_base64_decode,
     __binpac_filter_base64_close},
    {{0, 2},
     "GZIP",
     __binpac_filter_zlib_allocate,
     __binpac_filter_zlib_dtor,
     __binpac_filter_zlib_decode,
     __binpac_filter_zlib_close},
    {{0, 3},
     "ZLIB",
     __binpac_filter_zlib_allocate,
     __binpac_filter_zlib_dtor,
     __binpac_filter_zlib_decode,
     __binpac_filter_zlib_close},
    {{0, 0}, 0, 0, 0, 0},
};

void binpac_filter_dtor(hlt_type_info* ti, binpac_filter* filter, hlt_execution_context* ctx)
{
    // TODO: This is not consistnely called because HILTI is actually using
    // its own type info for the dummy struct type. We should unify that, but
    // it's not clear how .. For now, we can't rely on this running.
}

binpac_filter* binpachilti_filter_add(binpac_filter* head, hlt_enum ftype, hlt_exception** excpt,
                                      hlt_execution_context* ctx)
{
    binpac_filter* filter = 0;

    for ( __binpac_filter_definition* fd = filters; fd->name; fd++ ) {
        if ( hlt_enum_equal(ftype, fd->type, excpt, ctx) ) {
            filter = (*fd->allocate)(excpt, ctx);
            filter->def = fd;
            filter->next = 0;
            break;
        }
    }

    if ( ! filter ) {
        hlt_string fname = hlt_string_from_asciiz("unknown filter type", excpt, ctx);
        hlt_set_exception(excpt, &binpac_exception_filterunsupported, fname, ctx);
        return 0;
    }

    head = __binpachilti_filter_add(head, filter, excpt, ctx);
    return head;
}

binpac_filter* __binpachilti_filter_add(binpac_filter* head, binpac_filter* filter,
                                        hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! head )
        return filter;

    while ( head->next )
        head = head->next;

    head->next = filter;
    GC_CCTOR(filter, binpac_filter, ctx);

    return head;
}

void binpachilti_filter_close(binpac_filter* head, hlt_exception** excpt,
                              hlt_execution_context* ctx)
{
    if ( ! head )
        return;

    binpac_filter* n = 0;
    for ( binpac_filter* f = head; f; f = n ) {
        n = f->next;
        (*f->def->close)(f, excpt, ctx);
        (*f->def->dtor)(0, f, ctx);

        if ( f != head )
            // The reference to the first filter is owned by the struct.
            GC_CLEAR(f, binpac_filter, ctx);
    }
}

// We borrow this from sink.c
extern void binpac_dbg_deliver(binpac_sink* sink, hlt_bytes* data, binpac_filter* filter,
                               hlt_exception** excpt, hlt_execution_context* ctx);

hlt_bytes* binpachilti_filter_decode(binpac_filter* head, hlt_bytes* data, hlt_exception** excpt,
                                     hlt_execution_context* ctx) // &ref(!)
{
    binpac_filter* filter = head;

    if ( ! head )
        return data;

    while ( head ) {
        hlt_bytes* new_data = (*head->def->decode)(head, data, excpt, ctx);

        if ( hlt_bytes_is_frozen(data, excpt, ctx) )
            // No more data going to come.
            hlt_bytes_freeze(new_data, 1, excpt, ctx);

        if ( *excpt )
            return 0;

        binpac_dbg_deliver(0, new_data, filter, excpt, ctx);

        data = new_data;
        head = head->next;
    }

    return data;
}
