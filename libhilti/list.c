// $Id$
//
// The list implementation is pretty much a standard double-linked list, with
// an additional free-list to avoid allocating a new node for each element.
// However, we don't recycle earased nodes back into the free-list; that
// allows us to keep iterator valid even if a list changes. The GC will take
// of freeing nodes eventually.

#include "hilti.h"

#include <string.h>

struct hlt_timer_mgr;

// Initial allocation size for the free list.
static const uint32_t InitialCapacity = 2;

// Factor by which to growth free array when allocating more memory.
static const float GrowthFactor = 1.5;

struct __hlt_list_node {
    __hlt_list_node* prev;  // Predecessor node.
    __hlt_list_node* next;  // Successor node.
    // hlt_timer* timer;       // The entry's timer, or null if none is set.
    int64_t invalid;        // True if node has been invalidated.
                            // FIXME: int64_t to align the data; how else to do that?
    char data[];            // Node data starts here, with size determined by elem type.
};

struct __hlt_list {
    __hlt_list_node* head;       // First list element.
    __hlt_list_node* tail;       // Last list element.
    __hlt_list_node* free;       // First node is free array.
    uint16_t available;          // Number of nodes still available in free array.
    int64_t size;                // Current list size.
    const hlt_type_info* type;   // Element type.
    struct hlt_timer_mgr* tmgr;         // The timer manager or null.
    //hlt_interval timeout;        // The timeout value, or 0 if disabled
    //hlt_enum strategy;           // Expiration strategy if set; zero otherwise.
    int timeout;        // The timeout value, or 0 if disabled
    int strategy;
};

// Inserts n after node pos. If pos is null, inserts at front. n must be all
// zero-initialized.
static void _link(hlt_list* l, __hlt_list_node* n, __hlt_list_node* pos)
{
    if ( pos ) {
        if ( pos->next )
            pos->next->prev = n;
        else
            l->tail = n;

        n->next = pos->next;
        pos->next = n;
        n->prev = pos;
    }

    else {
        // Insert at head.
        n->next = l->head;
        if ( l->head )
            l->head->prev = n;
        else
            l->tail = n;
        l->head = n;
    }

    ++l->size;
}

// Unlinks the node from the list and invalidates it, including stopping its
// timer. Returns the same node n. 
static __hlt_list_node* _unlink(hlt_list* l, __hlt_list_node* n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( n->next )
        n->next->prev = n->prev;
    else
        l->tail = n->prev;

    if ( n->prev )
        n->prev->next = n->next;
    else
        l->head = n->next;

    n->prev = n->next = 0;
    n->invalid = 1;
    --l->size;

//    if ( n->timer && ctx )
//        hlt_timer_cancel(n->timer, excpt, ctx);

//    n->timer = 0;
    return n;
}

static __hlt_list_node* _make_node(hlt_list* l, void *val, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->available ) {
        // Need to allocate more nodes.
        int64_t c = (l->size+1) * GrowthFactor;

        l->free = hlt_calloc(c, sizeof(__hlt_list_node) + l->type->size);
        if ( ! l->free )
            return 0;

        l->available = c;
    }

    // Take one node from the free list.
    __hlt_list_node* n = l->free;
    l->free = ((void*)l->free) + sizeof(__hlt_list_node) + l->type->size;
    --l->available;

    // Init node (note that the memory is zero-initialized).
    memcpy(&n->data, val, l->type->size);

#if 0
    // Start timer if needed.
    if ( l->tmgr && l->timeout ) {
        __hlt_list_timer_cookie cookie = { l, n };
        n->timer = __hlt_timer_new_list(cookie, excpt, ctx);
        hlt_time t = hlt_timer_mgr_current(l->tmgr, excpt, ctx) + l->timeout;
        hlt_timer_mgr_schedule(l->tmgr, t, n->timer, excpt, ctx);
    }
#endif

    return n;
}

static inline void _access(hlt_list* l, __hlt_list_node* n, hlt_exception** excpt, hlt_execution_context* ctx)
{
#if 0
    if ( ! l->tmgr || ! hlt_enum_equal(l->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || l->timeout == 0 )
        return;

    if ( ! n->timer )
        return;

    hlt_time t = hlt_timer_mgr_current(l->tmgr, excpt, ctx) + l->timeout;
    hlt_timer_update(n->timer, t, excpt, ctx);
#endif
}

hlt_list* hlt_list_new(const hlt_type_info* elemtype, struct __hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list* l = hlt_malloc(sizeof(hlt_list));
    if ( ! l ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    l->free = hlt_calloc(InitialCapacity, sizeof(__hlt_list_node) + elemtype->size);
    if ( ! l->free ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    l->head = l->tail = 0;
    l->available = InitialCapacity;
    l->size = 0;
    l->type = elemtype;
    // l->tmgr = tmgr;
    l->timeout = 0.0;
    // l->strategy = hlt_enum_unset(excpt, ctx);

    return l;
}

#if 0
void hlt_list_timeout(hlt_list* l, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! l->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    l->timeout = timeout;
    l->strategy = strategy;
}
#endif

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

void hlt_list_erase(hlt_list_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
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

void hlt_list_insert(const hlt_type_info* type, void* val, hlt_list_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
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

hlt_list_iter hlt_list_begin(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list_iter i;
    i.list = l;
    i.node = l->head;
    return i;
}

hlt_list_iter hlt_list_end(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list_iter i;
    i.list = l;
    i.node = 0;
    return i;
}

hlt_list_iter hlt_list_iter_incr(hlt_list_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return i;
    }

    if ( ! i.node )
        // End of list.
        return i;

    hlt_list_iter j;
    j.list = i.list;
    j.node = i.node->next;

    return j;
}

void* hlt_list_iter_deref(const hlt_list_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.list || ! i.node || i.node->invalid ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    _access(i.list, i.node, excpt, ctx);
    return &i.node->data;
}

int8_t hlt_list_iter_eq(const hlt_list_iter i1, const hlt_list_iter i2, hlt_exception** excpt, hlt_execution_context* ctx)
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
