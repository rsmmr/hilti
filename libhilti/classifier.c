
#include "hilti.h"

typedef struct {
    int64_t priority;
    hlt_classifier_field** fields;
    void* value;
} hlt_classifier_rule;

struct __hlt_classifier {
    __hlt_gchdr __gchdr;   // Header for memory management.
    int64_t num_fields;
    const hlt_type_info* rule_type;
    const hlt_type_info* value_type;
    int8_t compiled;
    int64_t max_prio;

    int64_t num_rules;
    int64_t max_rules;
    hlt_classifier_rule** rules;
};

void hlt_classifier_dtor(hlt_type_info* ti, hlt_classifier* c)
{
    if ( ! rules )
        return;

    for ( int i = 0; i < c->num_rules; i++ ) {
        hlt_classifier_rule* r = c->rules[i];

        for ( int j = 0; j < c->num_fields; j++ )
            hlt_free(r->fields[j]);

        hlt_free(r->fields);
        GC_DTOR(r->value, c->vtype);
        hlt_free(r->value);
        hlt_free(r);
    }

    hlt_free(c->rules);
}

hlt_classifier* hlt_classifier_new(int64_t num_fields, const hlt_type_info* rtype, const hlt_type_info* vtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_classifier* c = GC_NEW(hlt_classifier);
    c->num_fields = num_fields;
    c->rule_type = rtype;
    c->value_type = vtype;
    c->compiled = 0;
    c->max_prio = 0;

    c->num_rules = 0;
    c->max_rules = 0;
    c->rules = 0;

    return c;
}

#ifdef DEBUG

static void dbg_print_fields(hlt_classifier* c, const char* func, hlt_classifier_field** fields)
{
    for ( int i = 0; i < c->num_fields; ++i ) {
        hlt_classifier_field* f = fields[i];

        char buffer[f->len * 3 + 1];
        for ( int j = 0; j < f->len; ++j )
            snprintf(buffer + j*3, 4, "%02x ", (int)f->data[j]);

        DBG_LOG("hilti-classifier", "%s:   [%d] len=%2lu/%3lu | %s", func, i, f->len, f->bits, buffer);
    }
}

#endif

// Does not ref copied element.
static inline void* _to_voidp(const hlt_type_info* type, void* data)
{
    void* z = hlt_malloc(type->size);
    memcpy(z, data, type->size);
    return z;
}

void hlt_classifier_add(hlt_classifier* c, hlt_classifier_field** fields, int64_t priority, const hlt_type_info* vtype, void* value, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( c->compiled ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    hlt_classifier_rule* r = hlt_malloc(sizeof(hlt_classifier_rule));
    r->priority = priority;
    r->fields = fields;
    r->value = _to_voidp(vtype, value);
    GC_CCTOR_GENERIC(r->valye, vtype);

    if ( c->num_rules >= c->max_rules ) {
        // Grow rule array.
        c->max_rules = (c->max_rules ? (int)(c->max_rules * 1.5) : 5);
        c->rules = (hlt_classifier_rule**) hlt_realloc(c->rules, c->max_rules * sizeof(hlt_classifier_rule));
    }

    c->rules[c->num_rules++] = r;

    if ( priority > c->max_prio )
        c->max_prio = priority;

#ifdef DEBUG
    DBG_LOG("hilti-classifier", "%s: new rule %p with priority %d for classifier %p", "classifier_add", r, priority, c);
    dbg_print_fields(c, "classifier_add", fields);
#endif
}

void hlt_classifier_add_no_prio(hlt_classifier* c, hlt_classifier_field** fields, const hlt_type_info* vtype, void* value, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_classifier_add(c, fields, c->max_prio + 1, vtype, value, excpt, ctx);
}

// Compare rules by priority for sorting.
static int cmp_rules(const void* p1, const void* p2)
{
    hlt_classifier_rule** r1 = (hlt_classifier_rule**) p1;
    hlt_classifier_rule** r2 = (hlt_classifier_rule**) p2;

    // Reverse sort.
    return ((*r2)->priority - (*r1)->priority);
}

void hlt_classifier_compile(hlt_classifier* c, hlt_exception** excpt, hlt_execution_context* ctx)
{
    c->compiled = 1;

    // Sort rules by priority.
    qsort(c->rules, c->num_rules, sizeof(hlt_classifier_rule*), cmp_rules);
}

static int8_t match_single_rule(hlt_classifier* c, hlt_classifier_rule* r, hlt_classifier_field** vals)
{
    for ( int i = 0; i < c->num_fields; i++ ) {
        hlt_classifier_field* field = r->fields[i];
        hlt_classifier_field* val = vals[i];

        if ( ! val->bits )
            // Wildcard in value.
            continue;

        if ( val->len < field->len )
            // Can't match.
            return 0;

        // Compare complete bytes.
        int n = field->bits / 8;
        if ( memcmp(&field->data, &val->data, n) != 0 )
            // No match.
            return 0;

        // Compare "fractional" bits.
        uint8_t bits = field->bits - (n*8);
        uint8_t mask = 0xff << (8 - bits);

        if ( (val->data[n] & mask) != (field->data[n] & mask) )
             // No match.
            return 0;
    }

    // All fields matched.
    return 1;
}

int8_t hlt_classifier_matches(hlt_classifier* c, hlt_classifier_field** vals, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! c->compiled ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

#ifdef DEBUG
    DBG_LOG("hilti-classifier", "%s: matching with classifier %p", "classifier_matches", c);
    dbg_print_fields(c, "classifier_matches", vals);
#endif

    for ( int i = 0; i < c->num_rules; i++ ) {
        if ( match_single_rule(c, c->rules[i], vals) ) {
            DBG_LOG("hilti-classifier", "%s: match found with rule %p", "classifier_matches", c->rules[i]);
            return 1;
        }
    }

    DBG_LOG("hilti-classifier", "%s: no match", "classifier_matches");
    return 0;
}

void* hlt_classifier_get(hlt_classifier* c, hlt_classifier_field** vals, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! c->compiled ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

#ifdef DEBUG
    DBG_LOG("hilti-classifier", "%s: matching with classifier %p", "classifier_get", c);
    dbg_print_fields(c, "classifier_get", vals);
#endif

    for ( int i = 0; i < c->num_rules; i++ ) {
        if ( match_single_rule(c, c->rules[i], vals) ) {
            DBG_LOG("hilti-classifier", "%s: match found with rule %p", "classifier_get", c->rules[i]);
            return c->rules[i]->value;
        }
    }

    DBG_LOG("hilti-classifier", "%s: no match", "classifier_get");
    hlt_set_exception(excpt, &hlt_exception_index_error, 0);
    return 0;
}

