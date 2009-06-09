// $Id$

#ifndef REGEXP_H
#define REGEXP_H

#include "hilti_intern.h"

typedef struct __hlt_regexp __hlt_regexp;

struct __hlt_regexp_range {
    __hlt_bytes_pos begin;
    __hlt_bytes_pos end;
};

typedef struct __hlt_regexp_range __hlt_regexp_range;

extern __hlt_regexp* __hlt_regexp_new(__hlt_exception* excpt);
extern void __hlt_regexp_compile(__hlt_regexp* re, const __hlt_string* pattern, __hlt_exception* excpt);

extern int8_t __hlt_regexp_string_find(__hlt_regexp* re, const __hlt_string* s, __hlt_exception* excpt);
extern int8_t __hlt_regexp_bytes_find(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt);

extern __hlt_regexp_range __hlt_regexp_string_span(__hlt_regexp* re, const __hlt_string* s, __hlt_exception* excpt);
extern __hlt_regexp_range __hlt_regexp_bytes_span(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt);

extern __hlt_vector *__hlt_regexp_string_groups(__hlt_regexp* re, const __hlt_string* s, __hlt_exception* excpt);
extern __hlt_vector *__hlt_regexp_bytes_groups(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt);

const __hlt_string* __hlt_regexp_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);

#endif
