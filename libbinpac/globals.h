
#ifndef LIBBINPAC_GLOBALS_H
#define LIBBINPAC_GLOBALS_H

#include "libbinpac++.h"

typedef struct {
    // Flag indicating __hlt_global_state_init() has completed.
    int initialized;

    // Flag indicating __hlt_global_state_done() has completed.
    int finished;

    hlt_list* parsers;
    hlt_map* mime_types;
    int8_t debugging;
} __binpac_global_state;

/// Initializes all global state. The function is called from binpac_init().
///
/// The function is safe against multiple executions; all calls after the
/// first will be ignored. Returns true if it was the first execution, and
/// false otherwise.
extern int __binpac_global_state_init();

/// Cleans up all global state. The function is called from binpac_done().
///
/// The function is safe against multiple executions; all calls after the
/// first will be ignored. Returns true if it was the first execution after
/// __binpac_global_state_init(), and false otherwise.
extern int __binpac_global_state_done();

/// Returns a pointer to the set of global state.
extern __binpac_global_state* __binpac_globals();

/// Returns true if the global state has already been initialized.
extern int __binpac_globals_initialized();

extern void* __binpac_runtime_state_get();
extern void __binpac_runtime_state_set(void* state);

#endif
