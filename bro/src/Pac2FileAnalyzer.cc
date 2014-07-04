
#include <memory.h>
#include <netinet/in.h>

#include <util/util.h>

extern "C" {
#include <libbinpac/libbinpac++.h>
}

#undef DBG_LOG

#include "Pac2FileAnalyzer.h"
#include "Plugin.h"
#include "Manager.h"
#include "LocalReporter.h"
#include "profile.h"

using namespace bro::hilti;
using namespace binpac;

using std::shared_ptr;

static inline void debug_msg(file_analysis::Analyzer* analyzer, const char* msg, int len, const u_char* data)
	{
#ifdef DEBUG
	if ( data )
		{
		PLUGIN_DBG_LOG(HiltiPlugin, "[%s/%lu/file] %s: |%s|",
			       file_mgr->GetComponentName(analyzer->Tag()).c_str(), analyzer->GetID(), msg,
			       fmt_bytes((const char*) data, min(40, len)), len > 40 ? "..." : "");
		}

	else
		{
		PLUGIN_DBG_LOG(HiltiPlugin, "[%s/%lu/file] %s",
			       file_mgr->GetComponentName(analyzer->Tag()).c_str(), analyzer->GetID(),
			       msg);
		}
#endif
	}

Pac2_FileAnalyzer::Pac2_FileAnalyzer(RecordVal* arg_args, file_analysis::File* arg_file)
	: file_analysis::Analyzer(arg_args, arg_file)
	{
	args = arg_args;
	Ref(args);

	cookie.type = Pac2Cookie::FILE;
	cookie.file_cookie.analyzer = this;
	cookie.file_cookie.tag = file_analysis::Tag(); // Error until we know it.
	}

Pac2_FileAnalyzer::~Pac2_FileAnalyzer()
	{
	Unref(args);
	cookie.file_cookie.analyzer = 0;
	}

void Pac2_FileAnalyzer::Init()
	{
	file_analysis::Analyzer::Init();

	cookie.file_cookie.tag = Tag();

	parser = 0;
	data = 0;
	resume = 0;
	skip = false;
	}

void Pac2_FileAnalyzer::Done()
	{
	file_analysis::Analyzer::Done();

	hlt_execution_context* ctx = hlt_global_execution_context();

	GC_DTOR(parser, hlt_BinPACHilti_Parser, ctx);
	GC_DTOR(data, hlt_bytes, ctx);
	GC_DTOR(resume, hlt_exception, ctx);
	}

int Pac2_FileAnalyzer::FeedChunk(int len, const u_char* chunk, bool eod)
	{
	// TODO: This is very similar to Pac2Analyzer.cc. Can we factor out common code?

	hlt_execution_context* ctx = hlt_global_execution_context();
	hlt_exception* excpt = 0;

	// If parser is set but not data, a previous parsing process has
	// finished. If so, we ignore all further input.
	if ( parser && ! chunk )
		{
		if ( len )
			debug_msg(this, "further data ignored", len, chunk);

		return 0;
		}

	if ( ! parser )
		{
		parser = HiltiPlugin.Mgr()->ParserForFileAnalyzer(Tag());

		if ( ! parser )
			{
			debug_msg(this, "no unit specificed for parsing", 0, 0);
			return 1;
			}

		GC_CCTOR(parser, hlt_BinPACHilti_Parser, ctx);
		}

	int result = 0;
	bool done = false;
	bool error = false;

	if ( ! data )
		{
		// First chunk.
		debug_msg(this, "initial chunk", len, chunk);

		data = hlt_bytes_new_from_data_copy((const int8_t*)chunk, len, &excpt, ctx);

		if ( eod )
			hlt_bytes_freeze(data, 1, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
		profile_update(PROFILE_HILTI_LAND, PROFILE_START);
#endif

		void* pobj = (*parser->parse_func)(data, &cookie, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
		profile_update(PROFILE_HILTI_LAND, PROFILE_STOP);
#endif

		GC_DTOR_GENERIC(&pobj, parser->type_info, ctx);
		}

	else
		{
		// Resume parsing.
		debug_msg(this, "resuming with chunk", len, chunk);

		assert(data && resume);

		if ( len )
			hlt_bytes_append_raw_copy(data, (int8_t*)chunk, len, &excpt, ctx);

		if ( eod )
			hlt_bytes_freeze(data, 1, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
		profile_update(PROFILE_HILTI_LAND, PROFILE_START);
#endif

		void* pobj = (*parser->resume_func)(resume, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
		profile_update(PROFILE_HILTI_LAND, PROFILE_STOP);
#endif
		GC_DTOR_GENERIC(&pobj, parser->type_info, ctx);
		resume = 0;
		}

	if ( excpt )
		{
		if ( hlt_exception_is_yield(excpt) )
			{
			debug_msg(this, "parsing yielded", 0, 0);
			resume = excpt;
			excpt = 0;
			result = -1;
			}

		else
			{
			// A parse error.
			hlt_exception* excpt2 = 0;
			char* e = hlt_exception_to_asciiz(excpt, &excpt2, ctx);
			assert(! excpt2);
			ParseError(e);
			hlt_free(e);
			GC_DTOR(excpt, hlt_exception, ctx);
			excpt = 0;
			error = true;
			result = 0;
			}
		}

	else // No exception.
		{
		done = true;
		result = 1;
		}

	// TODO: For now we just stop on error, later we might attempt to
	// restart parsing.
	if ( eod || done || error )
		data = 0; // Marker that we're done parsing.

	return result;
	}

bool Pac2_FileAnalyzer::DeliverStream(const u_char* data, uint64 len)
	{
	file_analysis::Analyzer::DeliverStream(data, len);

	if ( skip )
		return true;

	int rc = FeedChunk(len, data, false);

	if ( rc >= 0 )
		{
		debug_msg(this, ::util::fmt("parsing %s, skipping further content", (rc > 0 ? "finished" : "failed")).c_str(), 0, 0);
		skip = true;
		return rc > 0;
		}

	return true;
	}

bool Pac2_FileAnalyzer::Undelivered(uint64 offset, uint64 len)
	{
	return file_analysis::Analyzer::Undelivered(offset, len);
	}

bool Pac2_FileAnalyzer::EndOfFile()
	{
	file_analysis::Analyzer::EndOfFile();

	if ( skip )
		return true;

	int rc = FeedChunk(0, (const u_char*)"", true);
	return rc > 0;
	}

file_analysis::Analyzer* Pac2_FileAnalyzer::InstantiateAnalyzer(RecordVal* args, file_analysis::File* file)
	{
	return new Pac2_FileAnalyzer(args, file);
	}

void Pac2_FileAnalyzer::ParseError(const string& msg)
	{
	string s = "file parse error: " + msg;
	debug_msg(this, s.c_str(), 0, 0);
	reporter::weird(s);
	}
