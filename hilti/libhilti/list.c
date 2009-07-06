// $Id$
//
// The list implementation is pretty much a standard double-linked list, with
// an additional free-list to avoid allocating a new node for each element.
// However, we don't recycle earased nodes back into the free-list; that
// allows us to keep iterator valid even if a list changes. The GC will take
// of freeing nodes eventually.

#include <string.h>

#include "hilti_intern.h"

// Initial allocation size for the free list. 
static const uint32_t InitialCapacity = 2;

// Factor by which to growth free array when allocating more memory. 
static const float GrowthFactor = 1.5;

struct __hlt_list_node {
    __hlt_list_node* prev;  // Predecessor node. 
    __hlt_list_node* next;  // Successor node.
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
    const __hlt_type_info* type; // Element type.
};

// Inserts n after node pos. If pos is null, inserts at front. n must be all
// zero-initialized. 
static void _link(__hlt_list* l, __hlt_list_node* n, __hlt_list_node* pos)
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

// Unlinks the node from the list and invalidates it. Returns the same node
// n.
static __hlt_list_node* _unlink(__hlt_list* l, __hlt_list_node* n)
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
    return n;
}

static __hlt_list_node* _make_node(__hlt_list* l, void *val)
{
    if ( ! l->available ) {
        // Need to allocate more nodes. 
        int64_t c = (l->size+1) * GrowthFactor; 
            
        l->free = __hlt_gc_calloc_non_atomic(c, sizeof(__hlt_list_node) + l->type->size);
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
    return n;
}

__hlt_list* __hlt_list_new(const __hlt_type_info* elemtype, __hlt_exception* excpt)
{
    __hlt_list* l = __hlt_gc_malloc_non_atomic(sizeof(__hlt_list));
    if ( ! l ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }
    
    l->free = __hlt_gc_calloc_non_atomic(InitialCapacity, sizeof(__hlt_list_node) + elemtype->size);
    if ( ! l->free ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    l->head = l->tail = 0;
    l->available = InitialCapacity;
    l->size = 0;
    l->type = elemtype;
    return l;
}

void __hlt_list_push_front(__hlt_list* l, const __hlt_type_info* type, void* val, __hlt_exception* excpt)
{
    assert(type == l->type);
    
    __hlt_list_node* n = _make_node(l, val);
    if ( ! n ) {
        *excpt = __hlt_exception_out_of_memory;
        return;
    }
    
    _link(l, n, 0);
}

void __hlt_list_push_back(__hlt_list* l, const __hlt_type_info* type, void* val, __hlt_exception* excpt)
{
    assert(type == l->type);
    
    __hlt_list_node* n = _make_node(l, val);
    if ( ! n ) {
        *excpt = __hlt_exception_out_of_memory;
        return;
    }
    
    _link(l, n, l->tail);
}

void* __hlt_list_pop_front(__hlt_list* l, __hlt_exception* excpt)
{
    if ( ! l->head ) {
        *excpt = __hlt_exception_underflow;
        return 0;
    }
    
    return &(_unlink(l, l->head)->data);
}

void* __hlt_list_pop_back(__hlt_list* l, __hlt_exception* excpt)
{
    if ( ! l->tail ) {
        *excpt = __hlt_exception_underflow;
        return 0;
    }
    
    return &(_unlink(l, l->tail)->data);
}

void* __hlt_list_front(__hlt_list* l, __hlt_exception* excpt)
{
    if ( ! l->head ) {
        *excpt = __hlt_exception_underflow;
        return 0;
    }
    
    return &l->head->data;
}

void* __hlt_list_back(__hlt_list* l, __hlt_exception* excpt)
{
    if ( ! l->tail ) {
        *excpt = __hlt_exception_underflow;
        return 0;
    }
    
    return &l->tail->data;
}

int64_t __hlt_list_size(__hlt_list* l, __hlt_exception* excpt)
{
    return l->size;
}

void __hlt_list_erase(__hlt_list_iter i, __hlt_exception* excpt)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        *excpt = __hlt_exception_invalid_iterator;
        return;
    }
    
    if ( ! i.node ) {
        *excpt = __hlt_exception_invalid_iterator;
        return;
    }

    _unlink(i.list, i.node);
}

void __hlt_list_insert(const __hlt_type_info* type, void* val, __hlt_list_iter i, __hlt_exception* excpt)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        *excpt = __hlt_exception_invalid_iterator;
        return;
    }
    
    assert(type == i.list->type);
    
    __hlt_list_node* n = _make_node(i.list, val);
    if ( ! n ) {
        *excpt = __hlt_exception_out_of_memory;
        return;
    }

    if ( ! i.node )
        // Insert at end.
        _link(i.list, n, i.list->tail);
    else 
        _link(i.list, n, i.node->prev); 
}

__hlt_list_iter __hlt_list_begin(__hlt_list* l, __hlt_exception* excpt)
{
    __hlt_list_iter i; 
    i.list = l;
    i.node = l->head;
    return i;
}

__hlt_list_iter __hlt_list_end(__hlt_list* l, __hlt_exception* excpt)
{
    __hlt_list_iter i; 
    i.list = l;
    i.node = 0;
    return i;
}

__hlt_list_iter __hlt_list_iter_incr(__hlt_list_iter i, __hlt_exception* excpt)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        *excpt = __hlt_exception_invalid_iterator;
        return i;
    }
    
    if ( ! i.node )
        // End of list.
        return i;
    
    __hlt_list_iter j; 
    j.list = i.list;
    j.node = i.node->next;
    
    return j;
}

void* __hlt_list_iter_deref(const __hlt_list_iter i, __hlt_exception* excpt)
{
    if ( ! i.list || ! i.node || i.node->invalid ) {
        *excpt = __hlt_exception_invalid_iterator;
        return 0;
    }
    
    return &i.node->data;
}

int8_t __hlt_list_iter_eq(const __hlt_list_iter i1, const __hlt_list_iter i2, __hlt_exception* excpt)
{
    return i1.list == i2.list && i1.node == i2.node;
}

static __hlt_string_constant prefix = { 1, "[" };
static __hlt_string_constant postfix = { 1, "]" };
static __hlt_string_constant separator = { 1, "," };

__hlt_string __hlt_list_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    const __hlt_list* l = *((const __hlt_list**)obj);
    __hlt_string s = &prefix; 
    
    for ( __hlt_list_node* n = l->head; n; n = n->next ) {
        
        __hlt_string t;

        if ( l->type->to_string ) 
            t = (l->type->to_string)(l->type, &n->data, options, excpt);
        else
            // No format function.
            t = __hlt_string_from_asciiz(l->type->tag, excpt);

        if ( *excpt )
            return 0;
            
        s = __hlt_string_concat(s, t, excpt);
        if ( *excpt )
            return 0;
        
        if ( n->next ) {
            s = __hlt_string_concat(s, &separator, excpt);
            if ( *excpt )
                return 0;
        }
    }

    return __hlt_string_concat(s, &postfix, excpt);
    
}
