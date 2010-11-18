// $Id$
//
// Support functions for HILTI's integer data type.

#ifndef HILTI_INIT_H
#define HILTI_INIT_H

#include "exceptions.h"

// Initialize the HILTI run-time library.
extern void hlt_init();

// Registers a funtion to be executed when hilti_init() is run.
extern void hlt_register_init_function(void (*func)(), int8_t, hlt_exception* expt, hlt_execution_context* ctx);

#endif
