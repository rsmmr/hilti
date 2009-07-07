// $Id$

#ifndef HILTI_REGEXP_H
#define HILTI_REGEXP_H

#include "exceptions.h"
#include "bytes.h"
#include "vector.h"

typedef struct hlt_regexp hlt_regexp;
typedef struct hlt_regexp_range hlt_regexp_range;

struct hlt_regexp_range {
    hlt_bytes_pos begin;
    hlt_bytes_pos end;
};

extern hlt_regexp* hlt_regexp_new(hlt_exception* excpt);
extern void hlt_regexp_compile(hlt_regexp* re, hlt_string pattern, hlt_exception* excpt);

extern int8_t hlt_regexp_string_find(hlt_regexp* re, hlt_string s, hlt_exception* excpt);
extern int8_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt);

extern hlt_regexp_range hlt_regexp_string_span(hlt_regexp* re, hlt_string s, hlt_exception* excpt);
extern hlt_regexp_range hlt_regexp_bytes_span(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt);

extern hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, hlt_string s, hlt_exception* excpt);
extern hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt);

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt);

#endif
