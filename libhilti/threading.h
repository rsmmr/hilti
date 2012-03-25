
#ifndef HLT_THREADING_H
#define HLT_THREADING_H

#include "types.h"

/// Returns whether the HILTI runtime environment is configured for running
/// multiple threads.
///
/// Returns: True if configured for threaded execution.
extern int8_t hlt_is_multi_threaded();

#endif
