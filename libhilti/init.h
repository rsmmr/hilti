///
/// libhilti initialization code.
///

#ifndef LIBHILTI_INIT_H
#define LIBHILTI_INIT_H

/// Initializes the HILTI run-time library. Must be called once at startup
/// before any other libhilti functionality can be used. The one exception to
/// that rule is hlt_set_config(), which can (and must) be called before
/// hlt_init().
extern void hlt_init();

/// Terminates libhilti. Must be called before program termination.
/// Afterwards, no further libhilti functionality must be used anymore.
extern void hlt_done();

/// Internal version of libhilti that initialized the library from an
/// existign set of global state.
extern void __hlt_init_from_state(__hlt_global_state* state);

#endif
