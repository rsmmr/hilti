
#ifndef BRO_PLUGIN_HILTI_COOKIE_H
#define BRO_PLUGIN_HILTI_COOKIE_H

#include <analyzer/Analyzer.h>
#include <file_analysis/Analyzer.h>

namespace bro {
namespace hilti {

namespace pac2_cookie  {

struct Protocol {
	analyzer::Analyzer* analyzer;
	analyzer::Tag tag;
	bool is_orig;
	};

struct File {
	file_analysis::Analyzer* analyzer;
	file_analysis::Tag tag;
	};

}

struct Pac2Cookie {
	enum Type { PROTOCOL, FILE } type;
	pac2_cookie::Protocol protocol_cookie;
	pac2_cookie::File file_cookie;
	};

}
}

#endif
