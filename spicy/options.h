
#ifndef SPICY_OPTIONS_H
#define SPICY_OPTIONS_H

#include <hilti/options.h>

namespace spicy {

/// A set of options controlling the compilation and link process. This
/// inherits all the HILTi options and adds further Spicy-specific ones.
///
/// For the label lists, Spicy options are prefixed with "spicy".
class Options : public hilti::Options {
public:
    Options();
    Options(const Options& other) = default;
    virtual ~Options(){};

    /// List of directories to search for imports and other \c *.spicy library
    /// files. The current directory will always be tried first. By default,
    /// this set is set to the current directory plus the installation-wide
    /// HILTI library directories (in that order).
    string_list libdirs_spicy;

    /// True to generate parsing functions. Set by default.
    bool generate_parsers = true;

    /// True to generate composing functions. Unset by default.
    bool generate_composers = false;

    string_set cgDebugLabels() const override;
    string_set optimizationLabels() const override;
    void toCacheKey(::util::cache::FileCache::Key* key) const override;
};
}

#endif
