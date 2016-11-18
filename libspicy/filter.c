
#include "filter.h"
#include "exceptions.h"
#include "sink.h"

#include <autogen/spicyhilti-hlt.h>

__HLT_RTTI_GC_TYPE(spicy_filter, HLT_TYPE_SPICY_FILTER);

#include "filter_base64.c"
#include "filter_zlib.c"

// FIXME: Can't use the enums here directly; better way?
static struct __spicy_filter_definition filters[] = {
    {{0, 1},
     "BASE64",
     __spicy_filter_base64_allocate,
     __spicy_filter_base64_dtor,
     __spicy_filter_base64_decode,
     __spicy_filter_base64_close},
    {{0, 2},
     "GZIP",
     __spicy_filter_zlib_allocate,
     __spicy_filter_zlib_dtor,
     __spicy_filter_zlib_decode,
     __spicy_filter_zlib_close},
    {{0, 3},
     "ZLIB",
     __spicy_filter_zlib_allocate,
     __spicy_filter_zlib_dtor,
     __spicy_filter_zlib_decode,
     __spicy_filter_zlib_close},
    {{0, 0}, 0, 0, 0, 0},
};

void spicy_filter_dtor(hlt_type_info* ti, spicy_filter* filter, hlt_execution_context* ctx)
{
    // TODO: This is not consistnely called because HILTI is actually using
    // its own type info for the dummy struct type. We should unify that, but
    // it's not clear how .. For now, we can't rely on this running.
}

spicy_filter* spicyhilti_filter_add(spicy_filter* head, hlt_enum ftype, hlt_exception** excpt,
                                    hlt_execution_context* ctx)
{
    spicy_filter* filter = 0;

    for ( __spicy_filter_definition* fd = filters; fd->name; fd++ ) {
        if ( hlt_enum_equal(ftype, fd->type, excpt, ctx) ) {
            filter = (*fd->allocate)(excpt, ctx);
            filter->def = fd;
            filter->next = 0;
            break;
        }
    }

    if ( ! filter ) {
        hlt_string fname = hlt_string_from_asciiz("unknown filter type", excpt, ctx);
        hlt_set_exception(excpt, &spicy_exception_filterunsupported, fname, ctx);
        return 0;
    }

    head = __spicyhilti_filter_add(head, filter, excpt, ctx);
    return head;
}

spicy_filter* __spicyhilti_filter_add(spicy_filter* head, spicy_filter* filter,
                                      hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! head )
        return filter;

    while ( head->next )
        head = head->next;

    head->next = filter;
    GC_CCTOR(filter, spicy_filter, ctx);

    return head;
}

void spicyhilti_filter_close(spicy_filter* head, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! head )
        return;

    spicy_filter* n = 0;
    for ( spicy_filter* f = head; f; f = n ) {
        n = f->next;
        (*f->def->close)(f, excpt, ctx);
        (*f->def->dtor)(0, f, ctx);

        if ( f != head )
            // The reference to the first filter is owned by the struct.
            GC_CLEAR(f, spicy_filter, ctx);
    }
}

// We borrow this from sink.c
extern void spicy_dbg_deliver(spicy_sink* sink, hlt_bytes* data, spicy_filter* filter,
                              hlt_exception** excpt, hlt_execution_context* ctx);

hlt_bytes* spicyhilti_filter_decode(spicy_filter* head, hlt_bytes* data, hlt_exception** excpt,
                                    hlt_execution_context* ctx) // &ref(!)
{
    spicy_filter* filter = head;

    if ( ! head )
        return data;

    while ( head ) {
        hlt_bytes* new_data = (*head->def->decode)(head, data, excpt, ctx);

        if ( hlt_bytes_is_frozen(data, excpt, ctx) )
            // No more data going to come.
            hlt_bytes_freeze(new_data, 1, excpt, ctx);

        if ( *excpt )
            return 0;

        spicy_dbg_deliver(0, new_data, filter, excpt, ctx);

        data = new_data;
        head = head->next;
    }

    return data;
}
