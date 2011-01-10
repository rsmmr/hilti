// $Id$

#include <string.h>

#include "hilti.h"

typedef hlt_hash khint_t;
typedef void* __val_t;

typedef struct {
    __val_t val;       // The value stored in the map.
    hlt_timer* timer;  // The entry's timer, or null if none is set.
} __khval_map_t;

typedef hlt_timer* __khval_set_t; // The value stored for sets.

#include "3rdparty/khash/khash.h"

typedef struct __hlt_map {
    const hlt_type_info* tkey;   // Key type.
    const hlt_type_info* tvalue; // Value type.
    hlt_timer_mgr* tmgr;         // The timer manager or null.
    double timeout;              // The timeout value, or 0 if disabled
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.
    int8_t have_def;             // True if a default value has been set.
    __val_t def;                 // The default value if set.
    void *result;                // Cache for deref's result tuple.

    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    __khkey_t *keys;
    __khval_map_t *vals;
} kh_map_t;

struct __hlt_map_iter {
    hlt_map* map;  // Null if at end position.
    khiter_t iter;
};

typedef struct __hlt_set {
    const hlt_type_info* tkey;   // Key type.
    hlt_timer_mgr* tmgr;         // The timer manager or null.
    double timeout;              // The timeout value, or 0 if disabled
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.

    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    __khkey_t *keys;
    __khval_set_t *vals;
} kh_set_t;

struct __hlt_set_iter {
    hlt_set* set;  // Null if at end position.
    khiter_t iter;
};


hlt_hash hlt_default_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_hash_bytes(obj, type->size);
}

int8_t hlt_default_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return memcmp(obj1, obj2, type1->size) == 0;
}

static inline hlt_hash _kh_hash_func(const void* obj, const hlt_type_info* type)
{
    return (*type->hash)(type, obj, 0, 0);
}

static inline int8_t _kh_hash_equal(const void* obj1, const void* obj2, const hlt_type_info* type)
{
    return (*type->equal)(type, obj1, type, obj2, 0, 0);
}

static inline void* _to_voidp(const hlt_type_info* type, void* data)
{
    void* z = hlt_gc_malloc_non_atomic(type->size);
    memcpy(z, data, type->size);
    return z;
}

static inline void _access_map(hlt_map* m, khiter_t i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr || ! hlt_enum_equal(m->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || m->timeout == 0 )
        return;

    if ( ! kh_value(m, i).timer )
        return;

    double t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
    hlt_timer_update(kh_value(m, i).timer, t, excpt, ctx);
}

static inline void _access_set(hlt_set* m, khiter_t i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr || ! hlt_enum_equal(m->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || m->timeout == 0 )
        return;

    if ( ! kh_value(m, i) )
        return;

    double t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
    hlt_timer_update(kh_value(m, i), t, excpt, ctx);
}

//////////// Maps.

KHASH_INIT(map, __khkey_t, __khval_map_t, 1, _kh_hash_func, _kh_hash_equal)

hlt_map* hlt_map_new(const hlt_type_info* key, const hlt_type_info* value, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* m = kh_init(map);
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    m->tkey = key;
    m->tvalue = value;
    m->tmgr = tmgr;
    m->timeout = 0.0;
    m->strategy = hlt_enum_unset(excpt, ctx);
    m->have_def = 0;
    m->result = 0;
    return m;
}

void* hlt_map_get(hlt_map* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    khiter_t i = kh_get_map(m, key, type);

    if ( i == kh_end(m) ) {

        if ( m->have_def )
            return m->def;

        hlt_set_exception(excpt, &hlt_exception_index_error, 0);
        return 0;
    }

    _access_map(m, i, excpt, ctx);

    // It's unfortunate that we have look into the map here to get the size
    // of the value type because that's something the compiler can't optimize
    // away (as it should be able to in the other functions where it can get
    // to the size via a const parameter).
    return kh_value(m, i).val;
}

void* hlt_map_get_default(hlt_map* m, const hlt_type_info* tkey, void* key, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    khiter_t i = kh_get_map(m, key, tkey);

    if ( i == kh_end(m) )
        return def;

    _access_map(m, i, excpt, ctx);

    return kh_value(m, i).val;
}

void hlt_map_insert(hlt_map* m, const hlt_type_info* tkey, void* key, const hlt_type_info* tval, void* value, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    void* keytmp = _to_voidp(tkey, key);
    void* valtmp = _to_voidp(tval, value);

	int ret;
	khiter_t i = kh_put_map(m, keytmp, &ret, tkey);
    if ( ! ret )
        // Already exists, update timer.
        _access_map(m, i, excpt, ctx);

    else {
        // New entry.
        if ( m->tmgr && m->timeout ) {
            // Create timer.
            __hlt_map_timer_cookie cookie = { m, keytmp };
            kh_value(m, i).timer = __hlt_timer_new_map(cookie, excpt, ctx);
            double t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i).timer, excpt, ctx);
        }
        else
            kh_value(m, i).timer = 0;
    }

    kh_value(m, i).val = valtmp;
}

int8_t hlt_map_exists(hlt_map* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    khiter_t i = kh_get_map(m, key, type);
    if ( i == kh_end(m) )
        return 0;

    _access_map(m, i, excpt, ctx);
    return 1;
}

void hlt_map_remove(hlt_map* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    khiter_t i = kh_get_map(m, key, type);

    if ( i != kh_end(m) ) {
        if ( kh_value(m, i).timer ) {
            hlt_timer_cancel(kh_value(m, i).timer, excpt, ctx);
            kh_value(m, i).timer = 0;
        }

        kh_del_map(m, i);
    }
}

void hlt_list_map_expire(__hlt_map_timer_cookie cookie)
{
    khiter_t i = kh_get_map(cookie.map, cookie.key, cookie.map->tkey);

    if ( i == kh_end(cookie.map) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.map, i).timer = 0;
    kh_del_map(cookie.map, i);
}

int64_t hlt_map_size(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    return kh_size(m);
}

void hlt_map_clear(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) && kh_value(m, i).timer ) {
            hlt_timer_cancel(kh_value(m, i).timer, excpt, ctx);
            kh_value(m, i).timer = 0;
        }
    }

    kh_clear_map(m);
}

void hlt_map_default(hlt_map* m, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    m->have_def = 1;
    m->def = _to_voidp(tdef, def);
}

void hlt_map_timeout(hlt_map* m, hlt_enum strategy, double timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    m->timeout = timeout;
    m->strategy = strategy;
}

hlt_map_iter hlt_map_begin(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return hlt_map_end(excpt, ctx);
    }

    hlt_map_iter i;
    i.map = m;

    for ( i.iter = kh_begin(m); i.iter != kh_end(i.map); i.iter++ ) {
        if ( kh_exist(i.map, i.iter) )
            return i;
    }

    return hlt_map_end(excpt, ctx);
}

hlt_map_iter hlt_map_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map_iter i = { 0, 0 };
    return i;
}

hlt_map_iter hlt_map_iter_incr(hlt_map_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.map )
        // End already reached.
        return i;

    while ( i.iter != kh_end(i.map) ) {
        ++i.iter; // Don't do that inside kh_exit. It will be evaluated twice ...
        if ( kh_exist(i.map, i.iter) )
            return i;
    }

    return hlt_map_end(excpt, ctx);
}

void* hlt_map_iter_deref(const hlt_type_info* tuple, hlt_map_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.map ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    // Build return tuple.
    void* key = kh_key(i.map, i.iter);
    void* val = kh_value(i.map, i.iter).val;

    if ( ! i.map->result )
        i.map->result = hlt_gc_malloc_non_atomic(tuple->size);

    int16_t* offsets = (int16_t *)tuple->aux;
    hlt_type_info** types = (hlt_type_info**) &tuple->type_params;

    void *result = i.map->result;

    memcpy(result + offsets[0], key, types[0]->size);
    memcpy(result + offsets[1], val, types[1]->size);

    return result;
}

int8_t hlt_map_iter_eq(hlt_map_iter i1, hlt_map_iter i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.map == i2.map && i1.iter == i2.iter;
}

static hlt_string_constant prefix = { 2, "{ " };
static hlt_string_constant postfix = { 2, " }" };
static hlt_string_constant colon = { 2, ": " };
static hlt_string_constant separator = { 2, ", " };

hlt_string hlt_map_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_map* m = *((const hlt_map**)obj);
    hlt_string s = &prefix;
    int8_t first = 1;

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first )
            s = hlt_string_concat(s, &separator, excpt, ctx);

        hlt_string key;
        hlt_string value;

        if ( m->tkey->to_string )
            key = (m->tkey->to_string)(m->tkey, kh_key(m, i), options, excpt, ctx);
        else
            // No format function.
            key = hlt_string_from_asciiz(m->tkey->tag, excpt, ctx);

        if ( m->tvalue->to_string )
            value = (m->tvalue->to_string)(m->tvalue, kh_value(m, i).val, options, excpt, ctx);
        else
            // No format function.
            value = hlt_string_from_asciiz(m->tvalue->tag, excpt, ctx);

        if ( *excpt )
            return 0;

        s = hlt_string_concat(s, key, excpt, ctx);
        s = hlt_string_concat(s, &colon, excpt, ctx);
        s = hlt_string_concat(s, value, excpt, ctx);


        if ( *excpt )
            return 0;

        first = 0;
    }

    return hlt_string_concat(s, &postfix, excpt, ctx);

}
//////////// Sets.

KHASH_INIT(set, __khkey_t, __khval_set_t, 1, _kh_hash_func, _kh_hash_equal)

hlt_set* hlt_set_new(const hlt_type_info* key, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* m = kh_init(set);
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    m->tkey = key;
    m->tmgr = tmgr;
    m->timeout = 0.0;
    m->strategy = hlt_enum_unset(excpt, ctx);
    return m;
}

void hlt_set_insert(hlt_set* m, const hlt_type_info* tkey, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    void* keytmp = _to_voidp(tkey, key);

	int ret;
	khiter_t i = kh_put_set(m, keytmp, &ret, tkey);
    if ( ! ret )
        // Already exists, update timer.
        _access_set(m, i, excpt, ctx);

    else {
        // New entry.
        if ( m->tmgr && m->timeout ) {
            // Create timer.
            __hlt_set_timer_cookie cookie = { m, keytmp };
            kh_value(m, i) = __hlt_timer_new_set(cookie, excpt, ctx);
            double t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i), excpt, ctx);
        }
        else
            kh_value(m, i) = 0;
    }
}

int8_t hlt_set_exists(hlt_set* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    khiter_t i = kh_get_set(m, key, type);
    if ( i == kh_end(m) )
        return 0;

    _access_set(m, i, excpt, ctx);
    return 1;
}

void hlt_set_remove(hlt_set* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    khiter_t i = kh_get_set(m, key, type);

    if ( i != kh_end(m) ) {
        if ( kh_value(m, i) ) {
            hlt_timer_cancel(kh_value(m, i), excpt, ctx);
            kh_value(m, i) = 0;
        }

        kh_del_set(m, i);
    }
}

void hlt_list_set_expire(__hlt_set_timer_cookie cookie)
{
    khiter_t i = kh_get_set(cookie.set, cookie.key, cookie.set->tkey);

    if ( i == kh_end(cookie.set) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.set, i) = 0;
    kh_del_set(cookie.set, i);
}

int64_t hlt_set_size(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    return kh_size(m);
}

void hlt_set_clear(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) && kh_value(m, i) ) {
            hlt_timer_cancel(kh_value(m, i), excpt, ctx);
            kh_value(m, i) = 0;
        }
    }

    kh_clear_set(m);
}

void hlt_set_timeout(hlt_set* m, hlt_enum strategy, double timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    m->timeout = timeout;
    m->strategy = strategy;
}

hlt_set_iter hlt_set_begin(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return hlt_set_end(excpt, ctx);
    }

    hlt_set_iter i;
    i.set = m;

    for ( i.iter = kh_begin(m); i.iter != kh_end(i.set); i.iter++ ) {
        if ( kh_exist(i.set, i.iter) )
            return i;
    }

    return hlt_set_end(excpt, ctx);
}

hlt_set_iter hlt_set_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_iter i = { 0, 0 };
    return i;
}

hlt_set_iter hlt_set_iter_incr(hlt_set_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.set )
        // End already reached.
        return i;

    while ( i.iter != kh_end(i.set) ) {
        ++i.iter; // Don't do that inside kh_exit. It will be evaluated twice ...
        if ( kh_exist(i.set, i.iter) )
            return i;
    }

    return hlt_set_end(excpt, ctx);
}

void* hlt_set_iter_deref(hlt_set_iter i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.set ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    return kh_key(i.set, i.iter);
}

int8_t hlt_set_iter_eq(hlt_set_iter i1, hlt_set_iter i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.set == i2.set && i1.iter == i2.iter;
}

hlt_string hlt_set_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_set* m = *((const hlt_set**)obj);
    hlt_string s = &prefix;
    int8_t first = 1;

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first )
            s = hlt_string_concat(s, &separator, excpt, ctx);

        hlt_string key;

        if ( m->tkey->to_string )
            key = (m->tkey->to_string)(m->tkey, kh_key(m, i), options, excpt, ctx);
        else
            // No format function.
            key = hlt_string_from_asciiz(m->tkey->tag, excpt, ctx);

        s = hlt_string_concat(s, key, excpt, ctx);

        if ( *excpt )
            return 0;

        first = 0;
    }

    return hlt_string_concat(s, &postfix, excpt, ctx);
}
