///
/// libhilti initialization code.
///

#ifndef HILTI_INIT_H
#define HILTI_INIT_H

/// Initializes the HILTI run-time library. Must be called once at startup
/// before any other libhilti functionality can be used. The one exception to
/// that rule is hlt_set_config(), which can (and must) be called before
/// hlt_init().
extern void hlt_init();

/// Terminate libhilti. Must be called before program termination.
/// Afterwards, no further libhilti functionality must be used anymore.
extern void hlt_done();

#endif
