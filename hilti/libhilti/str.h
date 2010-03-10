// $Id$
// 
// Support functions for HILTI's string data type.

#ifndef HILTI_STRING_H
#define HILTI_STRING_H

#include "exceptions.h"
#include "bytes.h"
#include "rtti.h"

typedef int32_t hlt_string_size;

typedef struct __hlt_string hlt_string_constant;

struct __hlt_string {
    hlt_string_size len;
    int8_t bytes[];
} __attribute__((__packed__));

extern hlt_string hlt_string_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt);
extern hlt_string_size hlt_string_len(hlt_string s, hlt_exception** excpt);
extern hlt_string hlt_string_concat(hlt_string s1, hlt_string s2, hlt_exception** excpt);
extern hlt_string hlt_string_substr(hlt_string s1, hlt_string_size pos, hlt_string_size len, hlt_exception** excpt);
extern hlt_string_size hlt_string_find(hlt_string s, hlt_string pattern, hlt_exception** excpt);
extern int8_t hlt_string_cmp(hlt_string s1, hlt_string s2, hlt_exception** excpt);
extern hlt_string hlt_string_sprintf(hlt_string fmt, const hlt_type_info* type, void* (*tuple[]), hlt_exception** excpt);
extern hlt_bytes* hlt_string_encode(hlt_string s, hlt_string charset, hlt_exception** excpt);
extern hlt_string hlt_string_decode(hlt_bytes*, hlt_string charset, hlt_exception** excpt);

extern hlt_string hlt_string_empty(hlt_exception** excpt);
extern hlt_string hlt_string_from_asciiz(const char* asciiz, hlt_exception** excpt);
extern hlt_string hlt_string_from_data(const int8_t* data, hlt_string_size len, hlt_exception** excpt);
// Note that obj is a *pointer* to the object. 
extern hlt_string hlt_string_from_object(const hlt_type_info* type, void* obj, hlt_exception** excpt);

extern void hlt_string_print(FILE* file, hlt_string s, int8_t newline, hlt_exception** excpt);

extern const char* hlt_string_to_native(hlt_string s, hlt_exception** excpt);

#endif
