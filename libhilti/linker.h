
#ifndef LIBHILTI_LINKER_H
#define LIBHILTI_LINKER_H

#include <stdint.h>

extern void __hlt_modules_init(void* ctx);
extern void __hlt_globals_init(void* ctx);
extern void __hlt_globals_dtor(void* ctx);
extern int64_t __hlt_globals_size();

struct __hlt_execution_context;

struct __hlt_linker_functions {
    void (*__hlt_modules_init)(void* ctx);
    void (*__hlt_globals_init)(void* ctx);
    void (*__hlt_globals_dtor)(void* ctx);
    int64_t (*__hlt_globals_size)();
};

void __hlt_linker_set_functions(struct __hlt_linker_functions* funcs);

#endif
