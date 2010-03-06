// $Id$
//
// The list implementation is pretty much a standard double-linked list, with
// an additional free-list to avoid allocating a new node for each element.
// However, we don't recycle earased nodes back into the free-list; that
// allows us to keep iterator valid even if a list changes. The GC will take
// of freeing nodes eventually.

#include <string.h>

#include "hilti.h"

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
    const hlt_type_info* type; // Element type.
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

// Unlinks the node from the list and invalidates it. Returns the same node
// n.
static __hlt_list_node* _unlink(hlt_list* l, __hlt_list_node* n)
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

static __hlt_list_node* _make_node(hlt_list* l, void *val)
{
    if ( ! l->available ) {
        // Need to allocate more nodes. 
        int64_t c = (l->size+1) * GrowthFactor; 
            
        l->free = hlt_gc_calloc_non_atomic(c, sizeof(__hlt_list_node) + l->type->size);
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

hlt_list* hlt_list_new(const hlt_type_info* elemtype, hlt_exception** excpt)
{
    hlt_list* l = hlt_gc_malloc_non_atomic(sizeof(hlt_list));
    if ( ! l ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }
    
    l->free = hlt_gc_calloc_non_atomic(InitialCapacity, sizeof(__hlt_list_node) + elemtype->size);
    if ( ! l->free ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    l->head = l->tail = 0;
    l->available = InitialCapacity;
    l->size = 0;
    l->type = elemtype;
    return l;
}

void hlt_list_push_front(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt)
{
    assert(type == l->type);
    
    __hlt_list_node* n = _make_node(l, val);
    if ( ! n ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
    
    _link(l, n, 0);
}

void hlt_list_push_back(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt)
{
    assert(type == l->type);
    
    __hlt_list_node* n = _make_node(l, val);
    if ( ! n ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
    
    _link(l, n, l->tail);
}

void* hlt_list_pop_front(hlt_list* l, hlt_exception** excpt)
{
    if ( ! l->head ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }
    
    return &(_unlink(l, l->head)->data);
}

void* hlt_list_pop_back(hlt_list* l, hlt_exception** excpt)
{
    if ( ! l->tail ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }
    
    return &(_unlink(l, l->tail)->data);
}

void* hlt_list_front(hlt_list* l, hlt_exception** excpt)
{
    if ( ! l->head ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }
    
    return &l->head->data;
}

void* hlt_list_back(hlt_list* l, hlt_exception** excpt)
{
    if ( ! l->tail ) {
        hlt_set_exception(excpt, &hlt_exception_underflow, 0);
        return 0;
    }
    
    return &l->tail->data;
}

int64_t hlt_list_size(hlt_list* l, hlt_exception** excpt)
{
    return l->size;
}

void hlt_list_erase(hlt_list_iter i, hlt_exception** excpt)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }
    
    if ( ! i.node ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }

    _unlink(i.list, i.node);
}

void hlt_list_expire(hlt_list_iter i, hlt_exception** excpt)
{
    hlt_list_erase(i, excpt);
}

void hlt_list_insert(const hlt_type_info* type, void* val, hlt_list_iter i, hlt_exception** excpt)
{
    if ( (i.node && i.node->invalid) || ! i.list ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return;
    }
    
    assert(type == i.list->type);
    
    __hlt_list_node* n = _make_node(i.list, val);
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

hlt_list_iter hlt_list_begin(hlt_list* l, hlt_exception** excpt)
{
    hlt_list_iter i; 
    i.list = l;
    i.node = l->head;
    return i;
}

hlt_list_iter hlt_list_end(hlt_list* l, hlt_exception** excpt)
{
    hlt_list_iter i; 
    i.list = l;
    i.node = 0;
    return i;
}

hlt_list_iter hlt_list_iter_incr(hlt_list_iter i, hlt_exception** excpt)
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

void* hlt_list_iter_deref(const hlt_list_iter i, hlt_exception** excpt)
{
    if ( ! i.list || ! i.node || i.node->invalid ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }
    
    return &i.node->data;
}

int8_t hlt_list_iter_eq(const hlt_list_iter i1, const hlt_list_iter i2, hlt_exception** excpt)
{
    return i1.list == i2.list && i1.node == i2.node;
}

static hlt_string_constant prefix = { 1, "[" };
static hlt_string_constant postfix = { 1, "]" };
static hlt_string_constant separator = { 2, ", " };

hlt_string hlt_list_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt)
{
    const hlt_list* l = *((const hlt_list**)obj);
    hlt_string s = &prefix; 
    
    for ( __hlt_list_node* n = l->head; n; n = n->next ) {
        
        hlt_string t;

        if ( l->type->to_string ) 
            t = (l->type->to_string)(l->type, &n->data, options, excpt);
        else
            // No format function.
            t = hlt_string_from_asciiz(l->type->tag, excpt);

        if ( *excpt )
            return 0;
            
        s = hlt_string_concat(s, t, excpt);
        if ( *excpt )
            return 0;
        
        if ( n->next ) {
            s = hlt_string_concat(s, &separator, excpt);
            if ( *excpt )
                return 0;
        }
    }

    return hlt_string_concat(s, &postfix, excpt);
    
}
