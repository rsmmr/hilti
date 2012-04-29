//
// Factoring out some definitions from ffi64.c so that they can be included externally.
//
// -Robin
//

#ifndef LIBFFI_FFI64_H
#define LIBFFI_FFI64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#define _Bool bool
#endif

#define MAX_GPR_REGS 6
#define MAX_SSE_REGS 8

struct register_args
{
  /* Registers for argument passing.  */
  uint64_t gpr[MAX_GPR_REGS];
  __int128_t sse[MAX_SSE_REGS];
};

/* All reference to register classes here is identical to the code in
   gcc/config/i386/i386.c. Do *not* change one without the other.  */

/* Register class used for passing given 64bit part of the argument.
   These represent classes as documented by the PS ABI, with the exception
   of SSESF, SSEDF classes, that are basically SSE class, just gcc will
   use SF or DFmode move instead of DImode to avoid reformating penalties.

   Similary we play games with INTEGERSI_CLASS to use cheaper SImode moves
   whenever possible (upper half does contain padding).  */
enum x86_64_reg_class
  {
    X86_64_NO_CLASS,
    X86_64_INTEGER_CLASS,
    X86_64_INTEGERSI_CLASS,
    X86_64_SSE_CLASS,
    X86_64_SSESF_CLASS,
    X86_64_SSEDF_CLASS,
    X86_64_SSEUP_CLASS,
    X86_64_X87_CLASS,
    X86_64_X87UP_CLASS,
    X86_64_COMPLEX_X87_CLASS,
    X86_64_MEMORY_CLASS
  };

#define MAX_CLASSES 4

#define SSE_CLASS_P(X)	((X) >= X86_64_SSE_CLASS && X <= X86_64_SSEUP_CLASS)

extern int ffi64_examine_argument (ffi_type *type, enum x86_64_reg_class classes[MAX_CLASSES],
                                   _Bool in_return, int *pngpr, int *pnsse);

#ifdef __cplusplus
}
#endif

#endif
