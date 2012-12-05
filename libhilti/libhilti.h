/// Top-level HILTI include file.
///
/// All external code using libhilti should include this file, it exposes the
/// complete public interface. The one exception is C++ that intends to use
/// the JIT interface. It must instead include libhilti-jit.h.
///
/// However, the file should generally \a not be included directly by any
/// libhilti code. Instead, include only those headers that provided the
/// required functionality.

#ifndef LIBHILTI_H
#define LIBHILTI_H

#include "libhilti-intern.h"
#include "module/module.h"
#include "autogen/hilti-hlt.h"

#endif

