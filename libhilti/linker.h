
#ifndef LIBHILTI_LINKER_H
#define LIBHILTI_LINKER_H

#include <stdint.h>

#include "stackmap.h"

struct __hlt_execution_context;

extern void __hlt_modules_init(void* ctx);
extern void __hlt_globals_init(void* ctx);
extern void __hlt_globals_dtor(void* ctx);
extern int64_t __hlt_globals_size();
extern __llvm_stackmap* __hlt_llvm_stackmap();

// TODO: Rename to linker_data.
struct __hlt_linker_functions {
    void (*__hlt_modules_init)(void* ctx);
    void (*__hlt_globals_init)(void* ctx);
    void (*__hlt_globals_dtor)(void* ctx);
    int64_t (*__hlt_globals_size)();
    __llvm_stackmap* __stackmap;
};

void __hlt_linker_set_functions(struct __hlt_linker_functions* funcs);

#endif
