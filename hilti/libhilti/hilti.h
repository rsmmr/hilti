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

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

// Define here to avoid recursive dependencies.
typedef struct __hlt_string* hlt_string;

#include "addr.h"
#include "bool.h"
#include "bytes.h"
#include "channel.h"
#include "config.h"
#include "debug.h"
#include "double.h"
#include "enum.h"
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
#include "run.h"
#include "str.h"
#include "thread_context.h"
#include "tuple.h"
#include "utf8proc.h"
#include "vector.h"
#include "continuation.h"

#include "module/module.h"

#endif    
