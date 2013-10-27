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

int8_t hilti_is_compiled_bif(hlt_exception** excpt, hlt_execution_context* ctx)
	{
	return 1;
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
