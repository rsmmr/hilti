///
/// User-visible top-level include file.
///

#ifndef BINPAC_H
#define BINPAC_H

#include <fstream>

#include "common.h"
#include "context.h"

namespace binpac {

/// Initializes the BinPAC++ compiler infrastructure. Must be called before
/// any other method.
extern void init();

/// Returns the current BinPAC++ version.
extern string version();

}

#endif
