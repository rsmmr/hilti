///
/// User-visible top-level include file.
///

#ifndef SPICY_H
#define SPICY_H

#include <fstream>

#include "common.h"
#include "context.h"
#include "operator.h"
#include "options.h"

namespace spicy {

/// Initializes the Spicy compiler infrastructure. Must be called before
/// any other method.
extern void init();

/// Alias for init(), to avoid name clashes with the same function now part
/// of the classic Spicy implementation.
extern void init_spicy();

/// Returns the current Spicy version.
extern string version();

/// Returns a list of all Spicy operators.
operator_list operators();
}


#endif
