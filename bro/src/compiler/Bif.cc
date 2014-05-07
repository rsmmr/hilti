//
// Runtime functions supporting the generated HILTI/BinPAC++ code.
//
// These function all assume "HILTI-C" linkage. Note that they must all be
// 'extern "C"' here.
//
// All functions with names of the form <module>_<func>_bif will be
// automatically mapped to BiFs <module>::<func>. You must also declare them
// in scripts/bif (excluding the _bif postfix). If Bro has a legacy BiF
// function with the same name, providing one here will automatically overide
// that one.

#include "LocalReporter.h"
#include "RuntimeInterface.h"

extern "C" {

#include <libhilti/libhilti.h>
#include <autogen/libbro.hlt.h>

// This must match HILTI's struct layout.
// TODO: hiltic -P should be the one generating this.
// TODO: Copied here from Runtime.cc
struct hlt_LibBro_BroAny {
	__hlt_gchdr __gchdr;
	int32_t mask;
	//
	void* ptr;
	hlt_type_info* type_info;
	void* bro_type;
	void* to_val_func;
};

int8_t hilti_is_compiled_bif(hlt_exception** excpt, hlt_execution_context* ctx)
	{
	return 1;
	}

int64_t global_clear_table_bif(hlt_LibBro_BroAny* any, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	if ( any->type_info->type == HLT_TYPE_MAP )
			{
			hlt_map* m = *((hlt_map**) any->ptr);
			hlt_map_clear(m, excpt, ctx);
			}

	else if ( any->type_info->type == HLT_TYPE_SET )
			{
			hlt_set* m = *((hlt_set**) any->ptr);
			hlt_set_clear(m, excpt, ctx);
			}

	else
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);

    return 0;
	}

// FIXME: This is a hack because the compiler qualifies the name wrong...
int64_t dns_clear_table_bif(hlt_LibBro_BroAny* any, hlt_exception** excpt, hlt_execution_context* ctx)
	{
    return global_clear_table_bif(any, excpt, ctx);
	}

// The following function are for testing purposes only.
hlt_bytes* hilti_test_type_string_bif(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	GC_CCTOR(b, hlt_bytes);
	return b;
	}

hlt_bytes* global_mangle_string_bif(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	hlt_bytes_append_raw_copy(b, (int8_t*) "MangledGlobal", 13, excpt, ctx);
	GC_CCTOR(b, hlt_bytes);
	return b;
	}

hlt_bytes* hiltitesting_mangle_string_bif(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	hlt_bytes_append_raw_copy(b, (int8_t*) "MangledModule", 13, excpt, ctx);
	GC_CCTOR(b, hlt_bytes);
	return b;
	}

}
