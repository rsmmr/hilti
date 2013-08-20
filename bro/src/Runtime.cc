//
// Runtime functions supporting the generated HILTI/BinPAC++ code.
//
// These function all assume "HILTI-C" linkage.

#include "Runtime.h"
#include "LocalReporter.h"
#undef DBG_LOG

#include "Plugin.h"
#include "Conn.h"
#include "Val.h"
#include "Event.h"
#include "Rule.h"
#include "file_analysis/Manager.h"

#include "Pac2Analyzer.h"
#undef List

#include <hilti/context.h>
#include <autogen/bro.pac2.h>

namespace bro {
namespace hilti {

// When adding new runtime functions, you must extend this function table in
// RuntimeFunctionTable.cc, or the JIT compiler will not be able to find
// them.
extern const ::hilti::CompilerContext::FunctionMapping libbro_function_table[] = {
	{ "libbro_cookie_to_conn_val", (void*)&libbro_cookie_to_conn_val },
	{ "libbro_cookie_to_is_orig", (void*)&libbro_cookie_to_is_orig },
	{ "libbro_h2b_bytes", (void*)&libbro_h2b_bytes},
	{ "libbro_h2b_integer_signed", (void*)&libbro_h2b_integer_signed},
	{ "libbro_h2b_integer_unsigned", (void*)&libbro_h2b_integer_unsigned},
	{ "libbro_raise_event", (void*)&libbro_raise_event },
	{ "bro_file_begin", (void*)bro_file_begin },
	{ "bro_file_set_size", (void*)bro_file_set_size },
	{ "bro_file_data_in", (void*)bro_file_data_in },
	{ "bro_file_gap", (void*)bro_file_gap },
	{ "bro_file_end", (void*)bro_file_end },
	{ "bro_rule_match", (void*)bro_rule_match },
	{ 0, 0 } // End marker.
};

}
}

extern "C"  {

// Internal LibBro::* functions.

void* libbro_cookie_to_conn_val(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	return c->analyzer->Conn()->BuildConnVal();
	}

void* libbro_cookie_to_is_orig(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	return new Val(c->is_orig, TYPE_BOOL);
	}

void* libbro_h2b_bytes(hlt_bytes* value, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	int len = hlt_bytes_len(value, excpt, ctx);
	char data[len];
	hlt_bytes_to_raw_buffer(value, (int8_t*)data, len, excpt, ctx);
	return new StringVal(len, data); // copies data.
	}

void* libbro_h2b_integer_signed(int64_t i, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	return new Val(i, TYPE_INT);
	}

void* libbro_h2b_integer_unsigned(uint64_t i, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	return new Val(i, TYPE_COUNT);
	}

void libbro_raise_event(hlt_bytes* name, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	hlt_bytes_size len = hlt_bytes_len(name, excpt, ctx);
	char evname[len + 1];
	hlt_bytes_to_raw_buffer(name, (int8_t*)evname, len, excpt, ctx);
	evname[len] = '\0';

	EventHandlerPtr ev = event_registry->Lookup(evname);

	if ( ! ev.Ptr() )
		// No handler.
		return;

	int16_t* offsets = (int16_t *)type->aux;

	val_list* vals = new val_list;

	for ( int i = 0; i < type->num_params; i++ )
		{
		Val* broval = *((Val**)(((char*)tuple) + offsets[i]));
		vals->append(broval);
		}

	mgr.QueueEvent(ev, vals);
	}
}

// User-visible Bro::* functions.

void bro_file_begin(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	// Nothing to do.
	}

void bro_file_set_size(uint64_t size, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	file_mgr->SetSize(size, c->analyzer->GetAnalyzerTag(), c->analyzer->Conn(), c->is_orig);
	}

void bro_file_data_in(hlt_bytes* data, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;

	hlt_bytes_block block;
	hlt_iterator_bytes start = hlt_bytes_begin(data, excpt, ctx);
	hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

	void* bcookie = 0;

	while ( true )
		{
		bcookie = hlt_bytes_iterate_raw(&block, bcookie, start, end, excpt, ctx);

		file_mgr->DataIn((const u_char*)block.start,
				 block.end - block.start,
				 c->analyzer->GetAnalyzerTag(),
				 c->analyzer->Conn(),
				 c->is_orig);

		if ( ! bcookie )
			break;
		}

	GC_DTOR(start, hlt_iterator_bytes);
	GC_DTOR(end, hlt_iterator_bytes);
	}

void bro_file_data_in_at_offset(hlt_bytes* data, uint64_t offset, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;

	hlt_bytes_block block;
	hlt_iterator_bytes start = hlt_bytes_begin(data, excpt, ctx);
	hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

	void* bcookie = 0;

	while ( true )
		{
		bcookie = hlt_bytes_iterate_raw(&block, bcookie, start, end, excpt, ctx);

		file_mgr->DataIn((const u_char*)block.start,
				 block.end - block.start,
				 offset,
				 c->analyzer->GetAnalyzerTag(),
				 c->analyzer->Conn(),
				 c->is_orig);

		if ( ! bcookie )
			break;

		offset += (block.end - block.start);
		}

	GC_DTOR(start, hlt_iterator_bytes);
	GC_DTOR(end, hlt_iterator_bytes);
	}

void bro_file_gap(uint64_t offset, uint64_t len, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	file_mgr->Gap(offset, len, c->analyzer->GetAnalyzerTag(), c->analyzer->Conn(), c->is_orig);
	}

void bro_file_end(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	file_mgr->EndOfFile(c->analyzer->GetAnalyzerTag(), c->analyzer->Conn(), c->is_orig);
	}

void bro_dpd_confirm(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;
	c->analyzer->ProtocolConfirmation();
	}

void bro_rule_match(hlt_enum pattern_type, hlt_bytes* data, int8_t bol, int8_t eol, int8_t clear, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
	{
	assert(cookie);
	auto c = (bro::hilti::Pac2_Analyzer::Cookie*)cookie;

	Rule::PatternType bro_type = Rule::TYPES;

	if ( hlt_enum_equal(Bro_PatternType_FINGER, pattern_type, excpt, ctx) )
		bro_type = Rule::PAYLOAD;
	else if ( hlt_enum_equal(Bro_PatternType_FINGER, pattern_type, excpt, ctx) )
		bro_type = Rule::PAYLOAD;
	else if ( hlt_enum_equal(Bro_PatternType_HTTP_REQUEST, pattern_type, excpt, ctx) )
		bro_type = Rule::HTTP_REQUEST;
	else if ( hlt_enum_equal(Bro_PatternType_HTTP_REQUEST_BODY, pattern_type, excpt, ctx) )
		bro_type = Rule::HTTP_REQUEST_BODY;
	else if ( hlt_enum_equal(Bro_PatternType_HTTP_REQUEST_HEADER, pattern_type, excpt, ctx) )
		bro_type = Rule::HTTP_REQUEST_HEADER;
	else if ( hlt_enum_equal(Bro_PatternType_HTTP_REPLY_BODY, pattern_type, excpt, ctx) )
		bro_type = Rule::HTTP_REPLY_BODY;
	else if ( hlt_enum_equal(Bro_PatternType_HTTP_REPLY_HEADER, pattern_type, excpt, ctx) )
		bro_type = Rule::HTTP_REPLY_HEADER;
	else if ( hlt_enum_equal(Bro_PatternType_FTP, pattern_type, excpt, ctx) )
		bro_type = Rule::FTP;
	else if ( hlt_enum_equal(Bro_PatternType_FINGER, pattern_type, excpt, ctx) )
		bro_type = Rule::FINGER;
	else
		bro::hilti::reporter::internal_error("unknown pattern type in bro_rule_match()");

	hlt_bytes_block block;
	hlt_iterator_bytes start = hlt_bytes_begin(data, excpt, ctx);
	hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

	void* bcookie = 0;

	bool first = false;

	while ( true )
		{
		bcookie = hlt_bytes_iterate_raw(&block, bcookie, start, end, excpt, ctx);

		c->analyzer->Conn()->Match(bro_type,
					   (const u_char*)block.start,
					   block.end - block.start,
					   c->is_orig,
					   first ? bol : false,
					   bcookie ? false : eol,
					   first ? clear : false);

		if ( ! bcookie )
			break;

		first = false;
		}

	GC_DTOR(start, hlt_iterator_bytes);
	GC_DTOR(end, hlt_iterator_bytes);
	}

