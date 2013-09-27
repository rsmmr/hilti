
#include <vector>
#include <assert.h>

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
