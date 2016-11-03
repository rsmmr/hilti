
#ifndef BRO_PLUGIN_HILTI_COOKIE_H
#define BRO_PLUGIN_HILTI_COOKIE_H

#include <analyzer/Analyzer.h>
#include <file_analysis/Analyzer.h>

namespace bro {
namespace hilti {

namespace spicy_cookie {

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

struct SpicyCookie {
    enum Type { PROTOCOL, FILE } type;
    spicy_cookie::Protocol protocol_cookie;
    spicy_cookie::File file_cookie;
};
}
}

#endif
