///
/// libhilti initialization code.
///

#ifndef LIBHILTI_INIT_H
#define LIBHILTI_INIT_H

/// Initializes the HILTI run-time library. For non-JIT host applications,
/// this must be called once at startup before any other libhilti
/// functionality can be used. For JIT host applications, this is called
/// implicitly when after the code has been compiled; however, libhilti
/// functionality must be used only after that has happened. The one
/// exception to that rule in both cases is hlt_set_config(), which can (and
/// must) be called before hlt_init().
extern void hlt_init();

/// Internal function that shuts down the run-time library. Once executed, no
/// libhilti functioality can be used anymore. This is normally called
/// automatically at process termination, hlt_init() installes a
/// corresponding handler.
extern void __hlt_done();

/// Internal version of hlt_init() that does not install the atexit() handler
/// for __hlt_done().
extern void __hlt_init();

#endif
