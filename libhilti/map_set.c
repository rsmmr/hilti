
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

enum MapDefaultType {
    HLT_MAP_DEFAULT_NONE,
    HLT_MAP_DEFAULT_VALUE,
    HLT_MAP_DEFAULT_FUNCTION
};

typedef struct __hlt_map {
    __hlt_gchdr __gchdr;    // Header for memory management.
    const hlt_type_info* tkey;   // Key type.
    const hlt_type_info* tvalue; // Value type.
    hlt_timer_mgr* tmgr;         // The timer manager, or null if not used.
    hlt_interval timeout;        // The timeout value, or 0 if disabled
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.
    enum MapDefaultType default_type; // Type of the map's default.
    union {
        __val_t value;           // Default value for HLT_MAP_DEFAULT_VALUE
        hlt_callable* function;  // Default function for HLT_MAP_DEFAULT_FUNCTION
    } default_;

    void *cache_result;                // Cache for deref's result tuple.
    void *cache_default;               // Cache for DEFAULT_FUNCTION's result value.

    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    __khkey_t *keys;
    __khval_map_t *vals;
} kh_map_t;

typedef struct __hlt_set {
    __hlt_gchdr __gchdr;    // Header for memory management.
    const hlt_type_info* tkey;   // Key type.
    hlt_timer_mgr* tmgr;         // The timer manager, or null if not used.
    hlt_interval timeout;        // The timeout value, or 0 if disabled
    hlt_enum strategy;           // Expiration strategy if set; zero otherwise.

    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    __khkey_t *keys;
    __khval_set_t *vals;
} kh_set_t;

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

static inline void _map_clear_default(hlt_map* m, hlt_execution_context* ctx)
{
    switch ( m->default_type ) {
     case HLT_MAP_DEFAULT_NONE:
        break;

     case HLT_MAP_DEFAULT_VALUE:
        GC_DTOR_GENERIC(m->default_.value, m->tvalue, ctx);
        hlt_free(m->default_.value);
        break;

     case HLT_MAP_DEFAULT_FUNCTION:
        GC_DTOR(m->default_.function, hlt_callable, ctx);
        break;
    }

    m->default_type = HLT_MAP_DEFAULT_NONE;
    memset(&m->default_, 0, sizeof(m->default_)); // Just to help debugging.
}

void hlt_map_dtor(hlt_type_info* ti, hlt_map* m, hlt_execution_context* ctx)
{
    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) ) {

            if ( kh_value(m, i).timer ) {
                hlt_exception* excpt = 0;
                hlt_timer_cancel(kh_value(m, i).timer, &excpt, ctx);
            }

            GC_DTOR_GENERIC(kh_key(m, i), m->tkey, ctx);
            GC_DTOR_GENERIC(kh_value(m, i).val, m->tvalue, ctx);
            hlt_free(kh_key(m, i));
            hlt_free(kh_value(m, i).val);
        }
    }

    _map_clear_default(m, ctx);

    GC_DTOR(m->tmgr, hlt_timer_mgr, ctx);
    hlt_free(m->cache_result);
    hlt_free(m->cache_default);

    kh_destroy_map(m);
}

void hlt_iterator_map_cctor(hlt_type_info* ti, hlt_iterator_map* i, hlt_execution_context* ctx)
{
    GC_CCTOR(i->map, hlt_map, ctx);
}

void hlt_iterator_map_dtor(hlt_type_info* ti, hlt_iterator_map* i, hlt_execution_context* ctx)
{
    GC_DTOR(i->map, hlt_map, ctx);
}

void __hlt_map_timer_cookie_dtor(hlt_type_info* ti, __hlt_map_timer_cookie* c, hlt_execution_context* ctx)
{
    GC_DTOR(c->map, hlt_map, ctx);
}

void hlt_set_dtor(hlt_type_info* ti, hlt_set* s, hlt_execution_context* ctx)
{
    for ( khiter_t i = kh_begin(s); i != kh_end(s); i++ ) {
        if ( kh_exist(s, i) ) {

            if ( kh_value(s, i) ) {
                hlt_exception* excpt = 0;
                hlt_timer_cancel(kh_value(s, i), &excpt, ctx);
            }

            GC_DTOR_GENERIC(kh_key(s, i), s->tkey, ctx);
            hlt_free(kh_key(s, i));
        }
    }

    GC_DTOR(s->tmgr, hlt_timer_mgr, ctx);
    kh_destroy_set(s);
}

void hlt_iterator_set_cctor(hlt_type_info* ti, hlt_iterator_set* i, hlt_execution_context* ctx)
{
    GC_CCTOR(i->set, hlt_set, ctx);
}

void hlt_iterator_set_dtor(hlt_type_info* ti, hlt_iterator_set* i, hlt_execution_context* ctx)
{
    GC_DTOR(i->set, hlt_set, ctx);
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

static inline void _hlt_map_init(hlt_map* m, const hlt_type_info* key, const hlt_type_info* value, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    GC_INIT(m->tmgr, tmgr, hlt_timer_mgr, ctx);
    m->tkey = key;
    m->tvalue = value;
    m->timeout = 0.0;
    m->strategy = hlt_enum_unset(excpt, ctx);
    m->cache_result = 0;
    m->cache_default = 0;

    _map_clear_default(m, ctx);
}

hlt_map* hlt_map_new(const hlt_type_info* key, const hlt_type_info* value, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* m = GC_NEW(hlt_map, ctx);  // This would normally be kh_init(map); however we need to init the gc header.
    _hlt_map_init(m, key, value, tmgr, excpt, ctx);
    return m;
}

static void _clone_init_in_thread_map(const hlt_type_info* ti, void* dstp, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* dst = *(hlt_map**)dstp;

    // If we arrive here, it can't be a custom timer mgr but only the
    // thread-wide one.
    dst->tmgr = ctx->tmgr;
    GC_CCTOR(dst->tmgr, hlt_timer_mgr, ctx);

    for ( khiter_t i = kh_begin(dst); i != kh_end(dst); i++ ) {
        if ( ! kh_exist(dst, i) )
            continue;

        hlt_timer* t = kh_value(dst, i).timer;

        if ( ! t )
            continue;

        hlt_timer_mgr_schedule(dst->tmgr, t->time, t, excpt, ctx);
        GC_DTOR(t, hlt_timer, ctx); // Not memory-managed on our end.
    }
}

void* hlt_map_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GC_NEW_REF(hlt_map, ctx);
}

void hlt_map_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* src = *(hlt_map**)srcp;
    hlt_map* dst = *(hlt_map**)dstp;

    if ( src->tmgr && src->tmgr != ctx->tmgr ) {
        hlt_string msg = hlt_string_from_asciiz("map with non-standard timer mgr cannot be cloned", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_cloning_not_supported, msg, ctx);
        return;
    }

    dst->tmgr = 0; // set my init_in_thread()
    dst->tkey = src->tkey;
    dst->tvalue = src->tvalue;
    dst->timeout = src->timeout;
    dst->strategy = src->strategy;
    dst->default_type = src->default_type;
    dst->cache_result = 0;
    dst->cache_default = 0;

    switch ( src->default_type ) {
     case HLT_MAP_DEFAULT_NONE:
        break;

     case HLT_MAP_DEFAULT_VALUE:
        dst->default_.value = hlt_malloc(dst->tvalue->size);
        __hlt_clone(dst->default_.value, src->tvalue, src->default_.value, cstate, excpt, ctx);
        break;

     case HLT_MAP_DEFAULT_FUNCTION:
        dst->default_.function = hlt_malloc(dst->tvalue->size);
        __hlt_clone(dst->default_.function, &hlt_type_info_hlt_callable, src->default_.function, cstate, excpt, ctx);
        break;
    }

    for ( khiter_t i = kh_begin(src); i != kh_end(src); i++ ) {
        if ( ! kh_exist(src, i) )
            continue;

        void* key = hlt_malloc(src->tkey->size);
        void* val = hlt_malloc(src->tvalue->size);

        __hlt_clone(key, src->tkey, kh_key(src, i), cstate, excpt, ctx);
        __hlt_clone(val, src->tvalue, kh_value(src, i).val, cstate, excpt, ctx);

        int ret;
        khiter_t i = kh_put_map(dst, key, &ret, src->tkey);
        assert(ret); // Cannot exist yet.

        if ( src->tmgr && src->timeout ) {
            GC_CCTOR(dst, hlt_map, ctx);
            __hlt_map_timer_cookie cookie = { dst, key };
            hlt_timer* t = __hlt_timer_new_map(cookie, excpt, ctx);
            t->time = kh_value(src, i).timer->time;
            kh_value(dst, i).timer = t;
        }

        else
            kh_value(dst, i).timer = 0;

        kh_value(dst, i).val = val;
    }

    if ( src->tmgr )
        __hlt_clone_init_in_thread(_clone_init_in_thread_map, ti, dstp, cstate, excpt, ctx);
}

void* hlt_map_get(hlt_map* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    khiter_t i = kh_get_map(m, key, type);

    if ( i == kh_end(m) ) {

        switch ( m->default_type ) {
         case HLT_MAP_DEFAULT_NONE:
            break;

         case HLT_MAP_DEFAULT_VALUE:
            if ( ! m->cache_default )
                m->cache_default = hlt_malloc(m->tvalue->size);

            hlt_clone_deep(m->cache_default, m->tvalue, m->default_.value, excpt, ctx);
            return m->cache_default;

         case HLT_MAP_DEFAULT_FUNCTION: {
            if ( ! m->cache_default )
                m->cache_default = hlt_malloc(m->tvalue->size);

            void **k = (void **)key;
            HLT_CALLABLE_RUN(m->default_.function, m->cache_default, Hilti_MapDefaultFunction, k, excpt, ctx);
            return m->cache_default;
         }

        }

        hlt_set_exception(excpt, &hlt_exception_index_error, 0, ctx);
        return 0;
    }

    _access_map(m, i, excpt, ctx);

    return kh_value(m, i).val;
}

void* hlt_map_get_default(hlt_map* m, const hlt_type_info* tkey, void* key, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
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
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
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
        GC_DTOR_GENERIC(val, m->tvalue, ctx);
        hlt_free(val);

        // Update timer.
        _access_map(m, i, excpt, ctx);
    }

    else {
        // New entry.
        if ( m->tmgr && m->timeout ) {
            // Create timer.
            __hlt_map_timer_cookie cookie = { m, keytmp };
            kh_value(m, i).timer = __hlt_timer_new_map(cookie, excpt, ctx);
            hlt_time t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i).timer, excpt, ctx);
            GC_DTOR(kh_value(m, i).timer, hlt_timer, ctx); // Not memory-managed on our end.
        }
        else
            kh_value(m, i).timer = 0;

        GC_CCTOR_GENERIC(keytmp, m->tkey, ctx);
    }

    kh_value(m, i).val = valtmp;
    GC_CCTOR_GENERIC(valtmp, m->tvalue, ctx);
}

int8_t hlt_map_exists(hlt_map* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
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
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    khiter_t i = kh_get_map(m, key, type);

    if ( i != kh_end(m) ) {
        if ( kh_value(m, i).timer ) {
            hlt_timer_cancel(kh_value(m, i).timer, excpt, ctx);
            kh_value(m, i).timer = 0;
        }

        void* key = kh_key(m, i);
        GC_DTOR_GENERIC(key, m->tkey, ctx);
        hlt_free(key);

        void* val = kh_value(m, i).val;
        GC_DTOR_GENERIC(val, m->tvalue, ctx);
        hlt_free(val);

        kh_del_map(m, i);
    }
}

void hlt_map_expire(__hlt_map_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    khiter_t i = kh_get_map(cookie.map, cookie.key, cookie.map->tkey);

    if ( i == kh_end(cookie.map) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.map, i).timer = 0;

    void* key = kh_key(cookie.map, i);
    GC_DTOR_GENERIC(key, cookie.map->tkey, ctx);
    hlt_free(key);

    void* val = kh_value(cookie.map, i).val;
    GC_DTOR_GENERIC(val, cookie.map->tvalue, ctx);
    hlt_free(val);

    kh_del_map(cookie.map, i);
}

int64_t hlt_map_size(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    return kh_size(m);
}

void hlt_map_clear(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) ) {
            if ( kh_value(m, i).timer )
                hlt_timer_cancel(kh_value(m, i).timer, excpt, ctx);

            GC_DTOR_GENERIC(kh_key(m, i), m->tkey, ctx);
            GC_DTOR_GENERIC(kh_value(m, i).val, m->tvalue, ctx);
            hlt_free(kh_key(m, i));
            hlt_free(kh_value(m, i).val);
        }
    }

    kh_clear_map(m);
}

void hlt_map_default(hlt_map* m, const hlt_type_info* tdef, void* def, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _map_clear_default(m, ctx);

    if ( tdef->type != HLT_TYPE_CALLABLE )  {
        m->default_type = HLT_MAP_DEFAULT_VALUE;
        m->default_.value = hlt_malloc(m->tvalue->size);
        hlt_clone_deep(m->default_.value, m->tvalue, def, excpt, ctx);
    }

    else {
        m->default_type = HLT_MAP_DEFAULT_FUNCTION;
        m->default_.function = *(hlt_callable **)def;
        GC_CCTOR(m->default_.function, hlt_callable, ctx);
    }
}

void hlt_map_default_callable(hlt_map* m, hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _map_clear_default(m, ctx);

    m->default_type = HLT_MAP_DEFAULT_FUNCTION;
    m->default_.function = func;
    GC_CCTOR(m->default_.function, hlt_callable, ctx);
}

void hlt_map_timeout(hlt_map* m, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    m->timeout = timeout;
    m->strategy = strategy;

    if ( ! m->tmgr )
        GC_ASSIGN(m->tmgr, ctx->tmgr, hlt_timer_mgr, ctx);
}

hlt_iterator_map hlt_map_begin(hlt_map* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return hlt_map_end(excpt, ctx);
    }

    hlt_iterator_map i;

    for ( i.iter = kh_begin(m); i.iter != kh_end(m); i.iter++ ) {
        if ( kh_exist(m, i.iter) ) {
            i.map = m;
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
        if ( kh_exist(i.map, i.iter) )
            return i;
    }

    return hlt_map_end(excpt, ctx);
}

void* hlt_iterator_map_deref(const hlt_type_info* tuple, hlt_iterator_map i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.map ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0, ctx);
        return 0;
    }

    // Build return tuple.
    void* key = kh_key(i.map, i.iter);
    void* val = kh_value(i.map, i.iter).val;

    if ( ! i.map->cache_result )
        i.map->cache_result = hlt_malloc(tuple->size);

    int16_t* offsets = (int16_t *)tuple->aux;
    hlt_type_info** types = (hlt_type_info**) &tuple->type_params;

    void *result = i.map->cache_result;

    memcpy(result + offsets[0], key, types[0]->size);
    memcpy(result + offsets[1], val, types[1]->size);

    return result;
}

int8_t hlt_iterator_map_eq(hlt_iterator_map i1, hlt_iterator_map i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.map == i2.map && i1.iter == i2.iter;
}

hlt_string hlt_map_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_map* m = *((const hlt_map**)obj);

    if ( ! m )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    int8_t first = 1;

    hlt_string colon = hlt_string_from_asciiz(": ", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(", ", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("{ ", excpt, ctx);

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first )
            s = hlt_string_concat(s, separator, excpt, ctx);

        hlt_string key = __hlt_object_to_string(m->tkey, kh_key(m, i), options, seen, excpt, ctx);
        hlt_string value = __hlt_object_to_string(m->tvalue, kh_value(m, i).val, options, seen, excpt, ctx);

        s = hlt_string_concat(s, key, excpt, ctx);
        s = hlt_string_concat(s, colon, excpt, ctx);
        s = hlt_string_concat(s, value, excpt, ctx);

        if ( hlt_check_exception(excpt) )
            return 0;

        first = 0;
    }

    hlt_string postfix = hlt_string_from_asciiz(" }", excpt, ctx);
    s = hlt_string_concat(s, postfix, excpt, ctx);
    return s;
}

//////////// Sets.

static inline void _hlt_set_init(hlt_set* m, const hlt_type_info* key, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    GC_INIT(m->tmgr, tmgr, hlt_timer_mgr, ctx);
    m->tkey = key;
    m->timeout = 0.0;
    m->strategy = hlt_enum_unset(excpt, ctx);
}

hlt_set* hlt_set_new(const hlt_type_info* key, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* m = GC_NEW(hlt_set, ctx);  // This would normally be kh_init(set); however we need to init the gc header.
    _hlt_set_init(m, key, tmgr, excpt, ctx);
    return m;
}

static void _clone_init_in_thread_set(const hlt_type_info* ti, void* dstp, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* dst = *(hlt_set**)dstp;

    // If we arrive here, it can't be a custom timer mgr but only the
    // thread-wide one.
    dst->tmgr = ctx->tmgr;
    GC_CCTOR(dst->tmgr, hlt_timer_mgr, ctx);

    for ( khiter_t i = kh_begin(dst); i != kh_end(dst); i++ ) {
        if ( ! kh_exist(dst, i) )
            continue;

        hlt_timer* t = kh_value(dst, i);

        if ( ! t )
            continue;

        hlt_timer_mgr_schedule(dst->tmgr, t->time, t, excpt, ctx);
        GC_DTOR(t, hlt_timer, ctx); // Not memory-managed on our end.
    }
}

void* hlt_set_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GC_NEW(hlt_set, ctx);
}

void hlt_set_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* src = *(hlt_set**)srcp;
    hlt_set* dst = *(hlt_set**)dstp;

    if ( src->tmgr && src->tmgr != ctx->tmgr ) {
        hlt_string msg = hlt_string_from_asciiz("set with non-standard timer mgr cannot be cloned", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_cloning_not_supported, msg, ctx);
        return;
    }

    dst->tmgr = 0; // set my init_in_thread()
    dst->tkey = src->tkey;
    dst->timeout = src->timeout;
    dst->strategy = src->strategy;

    for ( khiter_t i = kh_begin(src); i != kh_end(src); i++ ) {
        if ( ! kh_exist(src, i) )
            continue;

        void* key = hlt_malloc(src->tkey->size);

        __hlt_clone(key, src->tkey, kh_key(src, i), cstate, excpt, ctx);

        int ret;
        khiter_t i = kh_put_set(dst, key, &ret, src->tkey);
        assert(ret); // Cannot exist yet.

        if ( src->tmgr && src->timeout ) {
            __hlt_set_timer_cookie cookie = { dst, key };
            hlt_timer* t = __hlt_timer_new_set(cookie, excpt, ctx);
            t->time = kh_value(src, i)->time;
            kh_value(dst, i) = t;
        }

        else
            kh_value(dst, i) = 0;
    }

    if ( src->tmgr )
        __hlt_clone_init_in_thread(_clone_init_in_thread_set, ti, dstp, cstate, excpt, ctx);
}

void hlt_set_insert(hlt_set* m, const hlt_type_info* tkey, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
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
            __hlt_set_timer_cookie cookie = { m, keytmp };
            kh_value(m, i) = __hlt_timer_new_set(cookie, excpt, ctx);
            hlt_interval t = hlt_timer_mgr_current(m->tmgr, excpt, ctx) + m->timeout;
            hlt_timer_mgr_schedule(m->tmgr, t, kh_value(m, i), excpt, ctx);
            GC_DTOR(kh_value(m, i), hlt_timer, ctx); // Not memory-managed on our end.
        }
        else
            kh_value(m, i) = 0;

        GC_CCTOR_GENERIC(keytmp, m->tkey, ctx);
    }
}

int8_t hlt_set_exists(hlt_set* m, const hlt_type_info* type, void* key, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
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
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    khiter_t i = kh_get_set(m, key, type);

    if ( i != kh_end(m) ) {
        if ( kh_value(m, i) ) {
            hlt_timer_cancel(kh_value(m, i), excpt, ctx);
            kh_value(m, i) = 0;
        }

        void* key = kh_key(m, i);
        GC_DTOR_GENERIC(key, m->tkey, ctx);
        hlt_free(key);

        kh_del_set(m, i);
    }
}

void hlt_set_expire(__hlt_set_timer_cookie cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    khiter_t i = kh_get_set(cookie.set, cookie.key, cookie.set->tkey);

    if ( i == kh_end(cookie.set) )
        // Removed in the mean-time, nothing to do.
        return;

    // Don't need to cancel the timer, as it has already expired anyway when
    // this method runs.
    kh_value(cookie.set, i) = 0;

    void* key = kh_key(cookie.set, i);
    GC_DTOR_GENERIC(key, cookie.set->tkey, ctx);
    hlt_free(key);

    kh_del_set(cookie.set, i);
}

int64_t hlt_set_size(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return 0;
    }

    return kh_size(m);
}

void hlt_set_clear(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( kh_exist(m, i) ) {
            if ( kh_value(m, i) )
                hlt_timer_cancel(kh_value(m, i), excpt, ctx);

            GC_DTOR_GENERIC(kh_key(m, i), m->tkey, ctx);
            hlt_free(kh_key(m, i));
        }
    }

    kh_clear_set(m);
}

void hlt_set_timeout(hlt_set* m, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx)
{
    m->timeout = timeout;
    m->strategy = strategy;

    if ( ! m->tmgr )
        GC_ASSIGN(m->tmgr, ctx->tmgr, hlt_timer_mgr, ctx);
}

hlt_iterator_set hlt_set_begin(hlt_set* m, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! m ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return hlt_set_end(excpt, ctx);
    }

    hlt_iterator_set i;

    for ( i.iter = kh_begin(m); i.iter != kh_end(m); i.iter++ ) {
        if ( kh_exist(m, i.iter) ) {
            i.set = m;
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
        if ( kh_exist(i.set, i.iter) )
            return i;
    }

    return hlt_set_end(excpt, ctx);
}

void* hlt_iterator_set_deref(hlt_iterator_set i, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! i.set ) {
        hlt_set_exception(excpt, &hlt_exception_invalid_iterator, 0, ctx);
        return 0;
    }

    return kh_key(i.set, i.iter);
}

int8_t hlt_iterator_set_eq(hlt_iterator_set i1, hlt_iterator_set i2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return i1.set == i2.set && i1.iter == i2.iter;
}

hlt_string hlt_set_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_set* m = *((const hlt_set**)obj);

    if ( ! m )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    int8_t first = 1;

    hlt_string separator = hlt_string_from_asciiz(", ", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("{ ", excpt, ctx);

    for ( khiter_t i = kh_begin(m); i != kh_end(m); i++ ) {
        if ( ! kh_exist(m, i) )
            continue;

        if ( ! first )
            s = hlt_string_concat(s, separator, excpt, ctx);

        hlt_string key = __hlt_object_to_string(m->tkey, kh_key(m, i), options, seen, excpt, ctx);
        s = hlt_string_concat(s, key, excpt, ctx);

        if ( hlt_check_exception(excpt) )
            return 0;

        first = 0;
    }

    hlt_string postfix = hlt_string_from_asciiz(" }", excpt, ctx);
    s = hlt_string_concat(s, postfix, excpt, ctx);

    return s;
}
