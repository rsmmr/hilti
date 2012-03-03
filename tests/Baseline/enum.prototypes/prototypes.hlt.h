
/* Automatically generated. Do not edit. */

#ifndef HILTI_MAIN_HLT_H
#define HILTI_MAIN_HLT_H

#include <hilti.h>

static const hlt_enum Main_Foo_A = { 0, 1 };
static const hlt_enum Main_Foo_BC = { 0, 2 };
static const hlt_enum Main_Foo_DEF = { 0, 3 };
void hlt_call_saved_callables(hlt_exception **);
void hlt_call_saved_callables_resume(const hlt_exception *, hlt_exception **);
void main_run(hlt_exception **);
void main_run_resume(const hlt_exception *, hlt_exception **);

#endif
