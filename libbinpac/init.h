///
/// libbinpac initialization code.
///

#ifndef LIBBINPAC_INIT_H
#define LIBBINPAC_INIT_H

/// Initializes the BinPAC++ run-time library. For non-JIT host applications,
/// this must be called once at startup before any other libbinpac
/// functionality can be used. For JIT host applications, this is called
/// implicitly after the code has been compiled; however, libhilti
/// functionality must be used only after that has happened.
extern void binpac_init();

/// Internal function that shuts down the runtime library. Once executed, no
/// libbinpac functioality can be used anymore. This is normally called
/// automatically at process termination, binpac_init() installs a
/// corresponding handler.
extern void __binpac_done();

/// Internal version binpac hlt_init() that does not install the atexit()
/// handler for __binpac_done().
extern void __binpac_init();

#endif
