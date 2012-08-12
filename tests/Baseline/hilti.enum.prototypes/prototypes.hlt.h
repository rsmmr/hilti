/* Automatically generated. Do not edit. */

#ifndef MAIN_HLT_H
#define MAIN_HLT_H

#include <libhilti.h>

static const hlt_enum Main_Foo_A = { 0, 1 };
static const hlt_enum Main_Foo_BC = { 0, 2 };
static const hlt_enum Main_Foo_DEF = { 0, 3 };
void main_run(hlt_exception** excpt);
void main_run_resume(hlt_exception* yield_excpt, hlt_exception** excpt);
#endif
