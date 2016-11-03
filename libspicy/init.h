///
/// libspicy initialization code.
///

#ifndef LIBSPICY_INIT_H
#define LIBSPICY_INIT_H

/// Initializes the Spicy run-time library. For non-JIT host applications,
/// this must be called once at startup before any other libspicy
/// functionality can be used. For JIT host applications, this is called
/// implicitly after the code has been compiled; however, libhilti
/// functionality must be used only after that has happened.
extern void spicy_init();

/// Internal function that shuts down the runtime library. Once executed, no
/// libspicy functioality can be used anymore. This is normally called
/// automatically at process termination, spicy_init() installs a
/// corresponding handler.
extern void __spicy_done();

/// Internal version of spicy_init() that does not install the atexit()
/// handler for __spicy_done().
extern void __spicy_init();

#endif
