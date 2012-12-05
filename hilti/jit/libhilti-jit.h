///
/// Top-level HILTI include file for C++ host functions that use the JIT
/// interface. These must include this header instead of \c libhilti.h as we
/// need to wrap some of the functions to instead call the JITed version.
/// (It's the functions that access any global libhilti state; this state may
/// access two times if we both JIT and independenlty link in libhilti. The
/// wrappers take care to use the right one).
///

#ifndef LIBHILTI_JIT_H
#define LIBHILTI_JIT_H

#ifndef __cplusplus
#error JIT support can only be used from C++ code.
#endif

#include <memory>

namespace hilti { class CompilerContext; }

/// Initializes the HILTI run-time library with JIT support. This must be
/// called instead of \a hlt_init() from host applications intending to use
/// the JIT. In a blatant violation of layering rules, the function also
/// takes care of calling \a binpac_init if available.
///
/// This is supported only for C++ applications.
///
/// \todo: We defined this inline here so that it's included directly into
/// the caller. That avoid linker errors with hlt_init_from_state() that I
/// haven't figured out otherwise yet.
extern void hlt_init_jit(std::shared_ptr<hilti::CompilerContext> ctx);

#endif
