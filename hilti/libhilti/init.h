// $Id$
// 
// Support functions for HILTI's integer data type.

#ifndef HILTI_INIT_H
#define HILTI_INIT_H

#include "exceptions.h"

// Initialize the HILTI run-time library. 
extern void hilti_init();

// Registers a funtion to be executed when hilti_init() is run.
extern void hlt_register_init_function(void (*func)(), hlt_exception* expt);

#endif
