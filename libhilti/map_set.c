
#include "autogen/hilti-hlt.h"
#include "map_set.h"
#include "timer.h"
#include "interval.h"
#include "enum.h"

#include <string.h>

typedef hlt_hash khint_t;
typedef void* __val_t;

typedef struct {
    __val_t val;       // The value stored in the map.
    hlt_timer* timer;  // The entry's timer, or null if none is set. Not memory-managed to avoid cycles.
} __khval_map_t;

typedef hlt_timer* __khval_set_t; // The value stored for sets. Not memory-managed to avoid cycles.

#include "3rdparty/khash/khash.h"

typedef struct __hlt_map {
    __hlt_gchdr __gchdr;    // Header for memory management.
    const hlt_type_info* tkey;   // Key type.
    const hlt_type_info* tvalue; // Value type.
    hlt_timer_mgr* tmgr;         // The timer manager or null.
    hlt_interval timeout;        // The timeout value, or 0 if disabled
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

struct __hlt_iterator_map {
    hlt_map* map;  // Null if at end position.
    khiter_t iter;
};

typedef struct __hlt_set {
    __hlt_gchdr __gchdr;    // Header for memory management.
    const hlt_type_info* tkey;   // Key type.
    hlt_timer_mgr* tmgr;         // The timer manager or null.
    hlt_interval timeout;        // The timeout value, or 0 if disabled
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.

    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    __khkey_t *keys;
    __khval_set_t *vals;
} kh_set_t;

struct __hlt_iterator_set {
    hlt_set* set;  // Null if at end position.
    khiter_t iter;
};

__HLT_RTTI_GC_TYPE(__hlt_map_timer_cookie,  HLT_TYPE_MAP_TIMER_COOKIE)
__HLT_RTTI_GC_TYPE(__hlt_set_timer_cookie,  HLT_TYPE_SET_TIMER_COOKIE)

static inline hlt_hash _kh_hash_func(const void* obj, const hlt_type_info* type)
{
    return (*type->hash)(type, obj, 0, 0);
}

static inline int8_t _kh_hash_equal(const void* obj1, const void* obj2, const hlt_type_info* type)
{
    return (*type->equal)(type, obj1, type, obj2, 0, 0);
}

KHASH_INIT(map, __khkey_t, __khval_map_t, 1, _kh_hash_func, _kh_hash_equal)
KHASH_INIT(set, __khkey_t, __khval_set_t, 1, _kh_hash_func, _kh_hash_equal)

void hlt_map_dtor(hlt_type_info* ti, hlt_map* m)
{
    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) ) {
            GC_DTOR_GENERIC(kh_key(m, i), m->tkey);
            GC_DTOR_GENERIC(kh_value(m, i).val, m->tvalue);
            hlt_free(kh_key(m, i));
            hlt_free(kh_value(m, i).val);
        }
    }

    GC_DTOR(m->tmgr, hlt_timer_mgr);
    GC_DTOR_GENERIC(m->def, m->tvalue);
    hlt_free(m->result);
    kh_destroy_map(m);
}

void hlt_iterator_map_cctor(hlt_type_info* ti, hlt_iterator_map* i)
{
    GC_CCTOR(i->map, hlt_map);
}

void hlt_iterator_map_dtor(hlt_type_info* ti, hlt_iterator_map* i)
{
    GC_DTOR(i->map, hlt_map);
}

void __hlt_map_timer_cookie_dtor(hlt_type_info* ti, __hlt_map_timer_cookie* c)
{
    GC_DTOR(c->map, hlt_map);
}

void hlt_set_dtor(hlt_type_info* ti, hlt_set* s)
{
    for ( khiter_t i = kh_begin(s); i != kh_end(s); i++ ) {
        if ( kh_exist(s, i) ) {
            GC_DTOR_GENERIC(kh_key(s, i), s->tkey);
            hlt_free(kh_key(s, i));
        }
    }

    GC_DTOR(s->tmgr, hlt_timer_mgr);
    kh_destroy_set(s);
}

void hlt_iterator_set_cctor(hlt_type_info* ti, hlt_iterator_set* i)
{
    GC_CCTOR(i->set, hlt_set);
}

void hlt_iterator_set_dtor(hlt_type_info* ti, hlt_iterator_set* i)
{
    GC_DTOR(i->set, hlt_set);
}

void __hlt_set_timer_cookie_dtor(hlt_type_info* ti, __hlt_set_timer_cookie* c)
{
    GC_DTOR(c->set, hlt_set);
}

hlt_hash hlt_default_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_hash hash = hlt_hash_bytes(obj, type->size);
    return hash;
}

int8_t hlt_default_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return memcmp(obj1, obj2, type1->size) == 0;
}

// Does not ref copied element.
static inline void* _to_voidp(const hlt_type_info* type, void* data)
{
    void* z = hlt_malloc(type->size);
    memcpy(z, data, type->size);
    return z;
}

static inline void _access_map(hlt_map* m, khiter_t i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr || ! hlt_enum_equal(m->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || m->timeout == 0 )
        return;

    if ( ! kh_value(m, i).timer )
        return;

    hlt_time t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
    hlt_timer_update(kh_value(m, i).timer, t, excpt, ctx);
}

static inline void _access_set(hlt_set* m, khiter_t i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr || ! hlt_enum_equal(m->strategy, Hilti_ExpireStrategy_Access, excpt, ctx) || m->timeout == 0 )
        return;

    if ( ! kh_value(m, i) )
        return;

    hlt_time t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
    hlt_timer_update(kh_value(m, i), t, excpt, ctx);
}

//////////// Maps.

hlt_map* hlt_map_new(const hlt_type_info* key, const hlt_type_info* value, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* m = GC_NEW(hlt_map);  // This would normally be kh_init(map); however we need to init the gc header.
    GC_INIT(m->tmgr, tmgr, hlt_timer_mgr);
    m->tkey = key;
    m->tvalue = value;
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

        if ( m->have_def ) {
            GC_CCTOR_GENERIC(m->def, m->tvalue);
            return m->def;
        }

        hlt_set_exception(excpt, &hlt_exception_index_error, 0);
        return 0;
    }

    _access_map(m, i, excpt, ctx);

    void* val = kh_value(m, i).val;
    GC_CCTOR_GENERIC(val, m->tvalue);
    return val;
}

void* hlt_map_get_default(hlt_map* m, const hlt_type_info* tkey, void* key, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return 0;
    }

    khiter_t i = kh_get_map(m, key, tkey);

    if ( i == kh_end(m) ) {
        GC_CCTOR_GENERIC(def, tdef);
        return def;
    }

    _access_map(m, i, excpt, ctx);

    void* val = kh_value(m, i).val;
    GC_CCTOR_GENERIC(val, m->tvalue);
    return val;
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

    if ( ! ret ) {
        // Entry already exists.

        // The hash table keeps the old key, so we don't need the new one.
        hlt_free(keytmp);

        // Delete the old value.
        void* val = kh_value(m, i).val;
        GC_DTOR_GENERIC(val, m->tvalue);
        hlt_free(val);

        // Update timer.
        _access_map(m, i, excpt, ctx);
    }

    else {
        // New entry.
        if ( m->tmgr && m->timeout ) {
            // Create timer.
            GC_CCTOR(m, hlt_map);
            __hlt_map_timer_cookie cookie = { m, keytmp };
            kh_value(m, i).timer = __hlt_timer_new_map(cookie, excpt, ctx);
            hlt_time t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i).timer, excpt, ctx);
            GC_DTOR(kh_value(m, i).timer, hlt_timer); // Not memory-managed on our end.
        }
        else
            kh_value(m, i).timer = 0;

        GC_CCTOR_GENERIC(keytmp, m->tkey);
    }

    kh_value(m, i).val = valtmp;
    GC_CCTOR_GENERIC(valtmp, m->tvalue);
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

        void* key = kh_key(m, i);
        GC_DTOR_GENERIC(key, m->tkey);
        hlt_free(key);

        void* val = kh_value(m, i).val;
        GC_DTOR_GENERIC(val, m->tvalue);
        hlt_free(val);

        kh_del_map(m, i);
    }
}

void hlt_map_expire(__hlt_map_timer_cookie cookie)
{
    khiter_t i = kh_get_map(cookie.map, cookie.key, cookie.map->tkey);

    if ( i == kh_end(cookie.map) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.map, i).timer = 0;

    void* key = kh_key(cookie.map, i);
    GC_DTOR_GENERIC(key, cookie.map->tkey);
    hlt_free(key);

    void* val = kh_value(cookie.map, i).val;
    GC_DTOR_GENERIC(val, cookie.map->tvalue);
    hlt_free(val);

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
        if ( kh_exist(m, i) ) {
            if ( kh_value(m, i).timer )
                hlt_timer_cancel(kh_value(m, i).timer, excpt, ctx);

            GC_DTOR_GENERIC(kh_key(m, i), m->tkey);
            GC_DTOR_GENERIC(kh_value(m, i).val, m->tvalue);
            hlt_free(kh_key(m, i));
            hlt_free(kh_value(m, i).val);
        }
    }

    kh_clear_map(m);
}

void hlt_map_default(hlt_map* m, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    m->have_def = 1;
    m->def = _to_voidp(tdef, def);
    GC_CCTOR_GENERIC(m->def, tdef);
}

void hlt_map_timeout(hlt_map* m, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    m->timeout = timeout;
    m->strategy = strategy;
}

hlt_iterator_map hlt_map_begin(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return hlt_map_end(excpt, ctx);
    }

    hlt_iterator_map i;

    for ( i.iter = kh_begin(m); i.iter != kh_end(m); i.iter++ ) {
        if ( kh_exist(m, i.iter) ) {
            GC_INIT(i.map, m, hlt_map);
            return i;
        }
    }

    return hlt_map_end(excpt, ctx);
}

hlt_iterator_map hlt_map_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_map i = { 0, 0 };
    return i;
}

hlt_iterator_map hlt_iterator_map_incr(hlt_iterator_map i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.map )
        // End already reached.
        return i;

    while ( i.iter != kh_end(i.map) ) {
        ++i.iter; // Don't do that inside kh_exit. It will be evaluated twice ...
        if ( kh_exist(i.map, i.iter) ) {
            GC_CCTOR(i, hlt_iterator_map);
            return i;
        }
    }

    return hlt_map_end(excpt, ctx);
}

void* hlt_iterator_map_deref(const hlt_type_info* tuple, hlt_iterator_map i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.map ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    // Build return tuple.
    void* key = kh_key(i.map, i.iter);
    void* val = kh_value(i.map, i.iter).val;

    if ( ! i.map->result )
        i.map->result = hlt_malloc(tuple->size);

    int16_t* offsets = (int16_t *)tuple->aux;
    hlt_type_info** types = (hlt_type_info**) &tuple->type_params;

    void *result = i.map->result;

    memcpy(result + offsets[0], key, types[0]->size);
    memcpy(result + offsets[1], val, types[1]->size);

    GC_CCTOR_GENERIC(key, i.map->tkey);
    GC_CCTOR_GENERIC(val, i.map->tvalue);

    return result;
}

int8_t hlt_iterator_map_eq(hlt_iterator_map i1, hlt_iterator_map i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.map == i2.map && i1.iter == i2.iter;
}

hlt_string hlt_map_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_map* m = *((const hlt_map**)obj);
    int8_t first = 1;

    hlt_string colon = hlt_string_from_asciiz(": ", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(", ", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("{ ", excpt, ctx);

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first ) {
            hlt_string tmp = s;
            s = hlt_string_concat(s, separator, excpt, ctx);
            GC_DTOR(tmp, hlt_string);
        }

        hlt_string key = 0;
        hlt_string value = 0;

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

        s = hlt_string_concat_and_unref(s, key, excpt, ctx);

        hlt_string tmp = s;
        s = hlt_string_concat(s, colon, excpt, ctx);
        GC_DTOR(tmp, hlt_string);

        s = hlt_string_concat_and_unref(s, value, excpt, ctx);

        if ( *excpt )
            goto error;

        first = 0;
    }

    hlt_string postfix = hlt_string_from_asciiz(" }", excpt, ctx);
    s = hlt_string_concat_and_unref(s, postfix, excpt, ctx);

    GC_DTOR(colon, hlt_string);
    GC_DTOR(separator, hlt_string);
    return s;

error:
    GC_DTOR(colon, hlt_string);
    GC_DTOR(separator, hlt_string);
    GC_DTOR(s, hlt_string);
    return 0;
}

//////////// Sets.

hlt_set* hlt_set_new(const hlt_type_info* key, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* m = GC_NEW(hlt_set);  // This would normally be kh_init(set); however we need to init the gc header.
    GC_INIT(m->tmgr, tmgr, hlt_timer_mgr);
    m->tkey = key;
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
    if ( ! ret ) {
        // The hash table keeps the old key, so we don't need the new one.
        hlt_free(keytmp);

        // Already exists, update timer.
        _access_set(m, i, excpt, ctx);
    }

    else {
        // New entry.
        if ( m->tmgr && m->timeout ) {
            // Create timer.
            GC_CCTOR(m, hlt_set);
            __hlt_set_timer_cookie cookie = { m, keytmp };
            kh_value(m, i) = __hlt_timer_new_set(cookie, excpt, ctx);
            hlt_interval t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i), excpt, ctx);
            GC_DTOR(kh_value(m, i), hlt_timer); // Not memory-managed on our end.
        }
        else
            kh_value(m, i) = 0;

        GC_CCTOR_GENERIC(keytmp, m->tkey);
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

        void* key = kh_key(m, i);
        GC_DTOR_GENERIC(key, m->tkey);
        hlt_free(key);

        kh_del_set(m, i);
    }
}

void hlt_set_expire(__hlt_set_timer_cookie cookie)
{
    khiter_t i = kh_get_set(cookie.set, cookie.key, cookie.set->tkey);

    if ( i == kh_end(cookie.set) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.set, i) = 0;

    void* key = kh_key(cookie.set, i);
    GC_DTOR_GENERIC(key, cookie.set->tkey);
    hlt_free(key);

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
        if ( kh_exist(m, i) ) {
            if ( kh_value(m, i) )
                hlt_timer_cancel(kh_value(m, i), excpt, ctx);

            GC_DTOR_GENERIC(kh_key(m, i), m->tkey);
            hlt_free(kh_key(m, i));
        }
    }

    kh_clear_set(m);
}

void hlt_set_timeout(hlt_set* m, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m->tmgr ) {
        hlt_set_exception(excpt, &hlt_exception_no_timer_manager, 0);
        return;
    }

    m->timeout = timeout;
    m->strategy = strategy;
}

hlt_iterator_set hlt_set_begin(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return hlt_set_end(excpt, ctx);
    }

    hlt_iterator_set i;

    for ( i.iter = kh_begin(m); i.iter != kh_end(m); i.iter++ ) {
        if ( kh_exist(m, i.iter) ) {
            GC_INIT(i.set, m, hlt_set);
            return i;
        }
    }

    return hlt_set_end(excpt, ctx);
}

hlt_iterator_set hlt_set_end(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_set i = { 0, 0 };
    return i;
}

hlt_iterator_set hlt_iterator_set_incr(hlt_iterator_set i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.set )
        // End already reached.
        return i;

    while ( i.iter != kh_end(i.set) ) {
        ++i.iter; // Don't do that inside kh_exit. It will be evaluated twice ...
        if ( kh_exist(i.set, i.iter) ) {
            GC_CCTOR(i, hlt_iterator_set);
            return i;
        }
    }

    return hlt_set_end(excpt, ctx);
}

void* hlt_iterator_set_deref(hlt_iterator_set i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.set ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0);
        return 0;
    }

    void* key = kh_key(i.set, i.iter);
    GC_CCTOR_GENERIC(key, i.set->tkey);
    return key;
}

int8_t hlt_iterator_set_eq(hlt_iterator_set i1, hlt_iterator_set i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.set == i2.set && i1.iter == i2.iter;
}

hlt_string hlt_set_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_set* m = *((const hlt_set**)obj);
    int8_t first = 1;

    hlt_string separator = hlt_string_from_asciiz(", ", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("{ ", excpt, ctx);

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first ) {
            hlt_string tmp = s;
            s = hlt_string_concat(s, separator, excpt, ctx);
            GC_DTOR(tmp, hlt_string);
        }

        hlt_string key = 0;

        if ( m->tkey->to_string )
            key = (m->tkey->to_string)(m->tkey, kh_key(m, i), options, excpt, ctx);
        else
            // No format function.
            key = hlt_string_from_asciiz(m->tkey->tag, excpt, ctx);

        s = hlt_string_concat_and_unref(s, key, excpt, ctx);

        if ( *excpt )
            goto error;

        first = 0;
    }

    hlt_string postfix = hlt_string_from_asciiz(" }", excpt, ctx);
    s = hlt_string_concat_and_unref(s, postfix, excpt, ctx);

    GC_DTOR(separator, hlt_string);
    return s;

error:
    GC_DTOR(separator, hlt_string);
    GC_DTOR(s, hlt_string);
    return 0;
}
