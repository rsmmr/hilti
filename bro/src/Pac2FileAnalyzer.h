
#ifndef BRO_PLUGIN_HILTI_PAC2FILEANALYZER_H
#define BRO_PLUGIN_HILTI_PAC2FILEANALYZER_H

#include "file_analysis/Manager.h"

#include "Cookie.h"

struct __binpac_parser;
struct __hlt_bytes;
struct __hlt_exception;

class Analyzer;

namespace bro {
namespace hilti {

class Pac2_FileAnalyzer : public file_analysis::Analyzer {
public:
	Pac2_FileAnalyzer(RecordVal* arg_args, file_analysis::File* arg_file);
	virtual ~Pac2_FileAnalyzer();

	void Init() override;
	void Done() override;
	bool DeliverStream(const u_char* data, uint64 len) override;
	bool Undelivered(uint64 offset, uint64 len) override;
	bool EndOfFile() override;

	static file_analysis::Analyzer* InstantiateAnalyzer(RecordVal* args, file_analysis::File* file);

protected:
	// Returns:
	//    -1: Parsing yielded waiting for more input.
	//     0: Parsing failed, not more input will be accepted.
	//     1: Parsing finished, not more input will be accepted.
	int FeedChunk(int len, const u_char* data, bool eod);

	void ParseError(const string& msg);

private:
	RecordVal* args;
	bool skip;
	Pac2Cookie cookie;

	__binpac_parser* parser;
	__hlt_bytes* data;
	__hlt_exception* resume;
};

}
}

#endif

