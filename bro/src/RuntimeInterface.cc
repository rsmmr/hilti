
#include <vector>
#include <assert.h>

extern "C"  {
#include <libhilti/libhilti.h>

#include "RuntimeInterface.h"

static std::vector<BroType *> type_table;

uint64_t lib_bro_add_indexed_type(BroType* type)
	{
	uint64_t idx = type_table.size();
	type_table.push_back(type);
	return idx;
	}

BroType* lib_bro_get_indexed_type(uint64_t idx)
	{
	assert(idx < type_table.size() && type_table[idx]);
	return type_table[idx];
	}

extern void libbro_object_mapping_unregister_bro(const ::BroObj* obj, hlt_exception** excpt, hlt_execution_context* ctx);
extern void libbro_object_mapping_invalidate_bro(const ::BroObj* obj, hlt_exception** excpt, hlt_execution_context* ctx);

void lib_bro_object_mapping_unregister_bro(const ::BroObj* obj)
	{
	hlt_exception* excpt = 0;
	hlt_execution_context* ctx = hlt_global_execution_context();

	return libbro_object_mapping_unregister_bro(obj, &excpt, ctx);
	}

void lib_bro_object_mapping_invalidate_bro(const ::BroObj* obj)
	{
	hlt_exception* excpt = 0;
	hlt_execution_context* ctx = hlt_global_execution_context();

	return libbro_object_mapping_invalidate_bro(obj, &excpt, ctx);
	}

}
