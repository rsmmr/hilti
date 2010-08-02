// $Id$
// 
// Top-level HILTI include file.

/// \mainpage LibHILTI Reference Documentation
/// 
/// \section data-types HILTI Data Types
/// 
/// - \ref bytes
/// - \ref regexp
/// 
/// \section run-time-support Run-Time Support
/// 
/// \section debugging Debugging Support
/// 
/// \section internals Internal Run-time Functions

#ifndef HILTI_H
#define HILTI_H

#define DEBUG

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

// gc.h must be included library-wide because it overwrites some standard
// functions (such as pthreads_create())
#ifdef DEBUG
#define GC_DEBUG
#endif

#include "3rdparty/bdwgc/include/gc.h"

#include "module/_hilti.h"
#include "types.h"

#include "addr.h"
#include "bool.h"
#include "bytes.h"
#include "channel.h"
#include "config.h"
#include "debug.h"
#include "double.h"
#include "enum.h"
#include "hook.h"
#include "exceptions.h"
#include "hilti.h"
#include "init.h"
#include "int.h"
#include "list.h"
#include "memory.h"
#include "net.h"
#include "overlay.h"
#include "port.h"
#include "regexp.h"
#include "str.h"
#include "threading.h"
#include "tuple.h"
#include "utf8proc.h"
#include "vector.h"
#include "continuation.h"
#include "list.h"
#include "timer.h"
#include "map_set.h"
#include "iosrc.h"
#include "context.h"
#include "globals.h"
#include "cmdqueue.h"
#include "file.h"

#include "module/module.h"

#endif    
