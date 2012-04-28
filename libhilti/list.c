//
// The list implementation is pretty much a standard double-linked list.
//
// NOTE: Unlike the old libhilti list implementation, there's no free list
// because that doesn't work well with ref'cnt. If we get a problem with to
// many small allocations, we could let the list do its own mem mgt though.

#include <string.h>

#include "list.h"

struct hlt_timer_mgr;

// Initial allocation size for the free list.
static const uint32_t InitialCapacity = 2;

// Factor by which to growth free array when allocating more memory.
static const float GrowthFactor = 1.5;

struct __hlt_list_node {
    __hlt_gchdr __gchdr;      // Header for memory management.
    __hlt_list_node* next;  // Successor node. Memory-managed.
    __hlt_list_node* prev;  // Predecessor node. Not memory-managed to avoid cycles.
    hlt_timer* timer;       // The entry's timer, or null if none is set.
    int64_t invalid;        // True if node has been invalidated.
                            // FIXME: int64_t to align the data; how else to do that?
    char data[];            // Node data starts here, with size determined by elem type.
};

struct __hlt_list {
    __hlt_gchdr __gchdr;      // Header for memory management.
    __hlt_list_node* head;       // First list element. Memory-managed.
    __hlt_list_node* tail;       // Last list element. Not memory-managed to avoid cycles.
    int64_t size;                // Current list size.
    const hlt_type_info* type;   // Element type.
    struct hlt_timer_mgr* tmgr;  // The timer manager or null.
    hlt_interval timeout;        // The timeout value, or 0 if disabled.
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.
};

// Inserts n after node pos. If pos is null, inserts at front. n must be all
// zero-initialized and already ref'ed.
static void _link(hlt_list* l, __hlt_list_node* n, __hlt_list_node* pos)
{
    if ( pos ) {
        if ( pos->next )
            pos->next->prev = n;
        else
            l->tail = n;

        GC_ASSIGN(n->next, pos->next, __hlt_list_node);
        GC_ASSIGN(pos->next, n, __hlt_list_node);
        n->prev = pos;
    }

    else {
        // Insert at head.
        GC_ASSIGN(n->next, l->head, __hlt_list_node);
        GC_ASSIGN(l->head, n, __hlt_list_node);

        if ( l->head )
            l->head->prev = n;
        else
            l->tail = n;
    }

    ++l->size;
}

// Unlinks the node from the list and invalidates it, including stopping its
// timer. Returns the same node n, which will still be ref'ed.
static __hlt_list_node* _unlink(hlt_list* l, __hlt_list_node* n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( n->next )
        n->next->prev = n->prev;
    else
        l->tail = n->prev;

    if ( n->prev )
        GC_ASSIGN(n->prev->next, n->next, __hlt_list_node);
    else
        GC_ASSIGN(l->head, n->next, __hlt_list_node);

    n->prev = n->next = 0;
    n->invalid = 1;
    --l->size;

    if ( n->timer && ctx )
        hlt_timer_cancel(n->timer, excpt, ctx);

    GC_CLEAR(n->timer, hlt_timer);
    return n;
}

/// Returns a ref'ed node.
static __hlt_list_node* _make_node(hlt_list* l, void *val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_list_node* = GC_NEW_CUSTOM_SIZE(__hlt_list_node, sizeof(__hlt_list_node) + l->type->size);

    // Init node (note that the memory is zero-initialized).
    memcpy(&n->data, val, l->type->size);

    // Start timer if needed.
    if ( l->tmgr && l->timeout ) {
        __hlt_list_timer_cookie cookie = { l, n };
        n->timer = __hlt_timer_new_list(cookie, excpt, ctx);
        hlt_time t = hlt_timer_mgr_current(l->tmgr, excpt, ctx) + l->timeout;
        hlt_timer_mgr_schedule(l->tmgr, t, n->timer, excpt, ctx);
    }

    return n;
}

static inline void _access(hlt_list* l, __hlt_list_node* n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->tmgr || ! hlt_enum_equal(l->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || l->timeout == 0 )
        return;

    if ( ! n->timer )
        return;

    hlt_time t = hlt_timer_mgr_current(l->tmgr, excpt, ctx) + l->timeout;
    hlt_timer_update(n->timer, t, excpt, ctx);
}

hlt_list* hlt_list_new(const hlt_type_info* elemtype, struct __hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list* l = GC_NEW(hlt_list);
    l->head = l->tail = 0;
    l->size = 0;
    l->type = elemtype;
    l->tmgr = tmgr;
    l->timeout = 0.0;
    l->strategy = hlt_enum_unset(excpt, ctx);
    return l;
}

void hlt_list_timeout(hlt_list* l, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    l->timeout = timeout;
    l->strategy = strategy;
}

void hlt_list_push_front(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type == l->type);

    __hlt_list_node* n = _make_node(l, val, excpt, ctx);
    if ( ! n ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }

    _link(l, n, 0);
}

void hlt_list_push_back(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type == l->type);

    __hlt_list_node* n = _make_node(l, val, excpt, ctx);
    if ( ! n ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }

    _link(l, n, l->tail);
}

void* hlt_list_pop_front(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->head ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }

    return &(_unlink(l, l->head, excpt, ctx)->data);
}

void* hlt_list_pop_back(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->tail ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }

    return &(_unlink(l, l->tail, excpt, ctx)->data);
}

void* hlt_list_front(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->head ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }

    _access(l, l->head, excpt, ctx);
    return &l->head->data;
}

void* hlt_list_back(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->tail ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }

    _access(l, l->tail, excpt, ctx);
    return &l->tail->data;
}

int64_t hlt_list_size(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return l->size;
}

void hlt_list_erase(hlt_list_iterator i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }

    if ( ! i.node ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }

    _unlink(i.list, i.node, excpt, ctx);
}

void hlt_list_list_expire(__hlt_list_timer_cookie cookie)
{
    _unlink(cookie.list, cookie.node, 0, 0);
}

void hlt_list_insert(const hlt_type_info* type, void* val, hlt_list_iterator i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }

    assert(type == i.list->type);

    __hlt_list_node* n = _make_node(i.list, val, excpt, ctx);
    if ( ! n ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }

    if ( ! i.node )
        // Insert at end.
        _link(i.list, n, i.list->tail);
    else
        _link(i.list, n, i.node->prev);
}

hlt_list_iterator hlt_list_begin(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list_iterator i;
    GC_INIT(i.list, l, hlt_list_iterator);
    GC_INIT(i.node, l->head, hlt_list_iterator);
    return i;
}

hlt_list_iterator hlt_list_end(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list_iterator i;
    GC_INIT(i.list, l, hlt_list_iterator);
    i.node = 0;
    return i;
}

hlt_list_iterator hlt_list_iterator_incr(hlt_list_iterator i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return i;
    }

    if ( ! i.node )
        // End of list.
        return i;

    hlt_list_iterator j;
    GC_INIT(j.list, i.list, hlt_list_iterator);
    GC_INIT(j.node, i.node->next, hlt_list_iterator);

    return j;
}

void* hlt_list_iterator_deref(const hlt_list_iterator i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.list || ! i.node || i.node->invalid ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    _access(i.list, i.node, excpt, ctx);
    return &i.node->data;
}

int8_t hlt_list_iterator_eq(const hlt_list_iterator i1, const hlt_list_iterator i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( i1.node )
        _access(i1.list, i1.node, excpt, ctx);

    if ( i2.node )
        _access(i2.list, i2.node, excpt, ctx);

    return i1.list == i2.list && i1.node == i2.node;
}


hlt_string hlt_list_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string prefix = hlt_string_from_asciiz("[", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz("]", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(",", excpt, ctx);

    const hlt_list* l = *((const hlt_list**)obj);
    hlt_string s = prefix;
    hlt_string result = 0;

    for ( __hlt_list_node* n = l->head; n; n = n->next ) {

        hlt_string t = 0;

        if ( l->type->to_string )
            t = (l->type->to_string)(l->type, &n->data, options, excpt, ctx);
        else
            // No format function.
            t = hlt_string_from_asciiz(l->type->tag, excpt, ctx);

        if ( *excpt )
            goto error;

        hlt_string tmp = s;
        s = hlt_string_concat(s, t, excpt, ctx);
        if ( *excpt )
            goto error;

        GC_DTOR(tmp, hlt_string);
        GC_DTOR(t, hlt_string);

        if ( n->next ) {
            hlt_string tmp = s;
            s = hlt_string_concat(s, separator, excpt, ctx);
            GC_DTOR(tmp, hlt_string);

            if ( *excpt )
                goto error;
        }
    }

    result = hlt_string_concat(s, postfix, excpt, ctx);

error:
    if ( s != prefix )
        GC_DTOR(s, hlt_string);

    GC_DTOR(prefix, hlt_string);
    GC_DTOR(postfix, hlt_string);
    GC_DTOR(separator, hlt_string);

    return result;
}
