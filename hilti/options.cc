
#include "options.h"
#include "util/util.h"
#include "hilti/autogen/hilti-config.h"

using namespace hilti;

Options::Options()
{
    optimizations = optimizationLabels();

    libdirs_hlt.push_back(".");

    for ( auto p : hilti::configuration().hilti_library_dirs )
        libdirs_hlt.push_back(p);
}

bool Options::optimizing(const string& label) const
{
    return optimizations.find(label) != optimizations.end();
}

bool Options::cgDebugging(const string& label) const
{
    return cg_debug.find(label) != cg_debug.end();
}

Options::string_set Options::cgDebugLabels() const
{
    return { "codegen", "linker", "parser", "scanner", "scopes", "context", "dump-ast", "print-ast", "visitors", "cache", "time", "liveness" };
}

Options::string_set Options::optimizationLabels() const
{
    return string_set();
}

void Options::toCacheKey(::util::cache::FileCache::Key* key) const
{
    key->options += (debug ? "D" : "d");
    key->options += (optimize ? "O" : "o");
    key->options += (profile ? ::util::fmt("P%d", profile) : "p");
    key->options += (verify ? "V" : "v");

    for ( auto d : libdirs_hlt )
        key->dirs.insert(d);

    for ( auto o : optimizations )
        key->hashes.insert(o);
}
