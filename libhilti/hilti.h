/// Top-level HILTI include file.
///
/// All external code using libhilti should include this file. It exposes the
/// complete public interface.
///
/// However, the file should generally \a not be included directly by any
/// libhilti code. Instead, include only those headers that provided the
/// required functionality.

#ifndef HILTI_H
#define HILTI_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "types.h"
#include "config.h"
#include "context.h"
#include "globals.h"
#include "debug.h"
#include "memory.h"
#include "init.h"
#include "string_.h"
#include "list.h"
#include "addr.h"
#include "enum.h"
#include "fiber.h"

// Not yet ported to new compiler framework.

#if 0
#include "addr.h"
#include "bool.h"
#include "bytes.h"
#include "channel.h"
#include "config.h"
#include "double.h"
#include "enum.h"
#include "hook.h"
#include "exceptions.h"
#include "debug.h"
#include "hilti.h"
#include "init.h"
#include "int.h"
#include "list.h"
#include "memory.h"
#include "net.h"
#include "overlay.h"
#include "port.h"
#include "regexp.h"
#include "threading.h"
#include "tuple.h"
#include "utf8proc.h"
#include "vector.h"
#include "continuation.h"
#include "list.h"
#include "tim.h"
#include "interval.h"
#include "timer.h"
#include "map_set.h"
#include "iosrc.h"
#include "context.h"
#include "globals.h"
#include "cmdqueue.h"
#include "file.h"
#include "profiler.h"
#include "classifier.h"
#include "util.h"

#include "module/module.h"

#include <hilti.hlt.h>

#endif

#endif
