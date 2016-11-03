
#include "options.h"
#include "spicy/autogen/spicy-config.h"

using namespace spicy;

Options::Options()
{
    // Make sure we call the overridden version here.
    optimizations = optimizationLabels();

    libdirs_spicy.push_back(".");

    for ( auto p : spicy::configuration().spicy_library_dirs ) {
        libdirs_spicy.push_back(p);
        libdirs_hlt.push_back(p);
    }
}

Options::string_set Options::cgDebugLabels() const
{
    return hilti::Options::cgDebugLabels();
}

Options::string_set Options::optimizationLabels() const
{
    return hilti::Options::optimizationLabels();
}

void Options::toCacheKey(::util::cache::FileCache::Key* key) const
{
    hilti::Options::toCacheKey(key);

    for ( auto d : libdirs_spicy )
        key->dirs.insert(d);
}
