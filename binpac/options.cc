
#include "options.h"
#include "autogen/binpac-config.h"

using namespace binpac;

Options::Options()
{
    // Make sure we call the overridden version here.
    optimizations = optimizationLabels();

    libdirs_pac2.push_back(".");

    for ( auto p : binpac::configuration().binpac_library_dirs ) {
        libdirs_pac2.push_back(p);
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

    for ( auto d : libdirs_pac2 )
        key->dirs.push_back(d);
}
