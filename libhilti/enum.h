//
// Support functions for HILTI's enum data type.
//

#ifndef LIBHILTI_ENUM_H
#define LIBHILTI_ENUM_H

#include "exceptions.h"

// We use __packed__ struct so that we don't end up with unitialized bytes
// that would require a dedicated hash function.
//
// TODO: We also use 16 bits for the flags because otherwise out ABI code
// doesn't seem to pass them correctly by value. *sigh* With that, we
// wouldn't actually need the packed anymore now ....
typedef struct hlt_enum {
    int64_t flags;
    int64_t value;
} __attribute__((__packed__)) hlt_enum;

#define HLT_ENUM_UNDEF   1
#define HLT_ENUM_HAS_VAL 2

#define hlt_enum_undefined(e) (e.flags & HLT_ENUM_UNDEF)
#define hlt_enum_has_val(e)   (e.flags & HLT_ENUM_HAS_VAL)

extern int64_t hlt_enum_value(hlt_enum e, hlt_exception** excpt, hlt_execution_context* ctx);
extern hlt_string hlt_enum_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_enum_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

extern hlt_enum hlt_enum_unset(hlt_exception** excpt, hlt_execution_context* ctx);
extern int8_t hlt_enum_equal(hlt_enum e1, hlt_enum e2, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
