// $Id$
//
// Internal BinPAC++ API funnctionaly supporting the generated parsers.

#include <stdio.h>
#include <stdlib.h>

#include "binpac.h"

static hlt_list* _parsers = 0;
static int _initalized = 0;

// FIXME: This is a very unfortunate naming scheme ...
extern const hlt_type_info hlt_type_info_struct_string_name_string_description_caddr_parse_func_caddr_resume_func;

static void _ensure_parsers(hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! _parsers )
        _parsers = hlt_list_new(&hlt_type_info_struct_string_name_string_description_caddr_parse_func_caddr_resume_func, excpt, ctx);
}

// Public API functions.

void binpac_init()
{
    _initalized = 1;
}

hlt_list* binpac_parsers(hlt_exception** excpt, hlt_execution_context* ctx)
{
    _ensure_parsers(excpt, ctx);
    return _parsers;
}

// Internal functions. 
 
void binpac_fatal_error(const char* msg)
{
    fprintf(stderr, "fatal binpac error: %s", msg);
}

// Note that this function can be called before binpac_init(). 
void binpacintern_register_parser(binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _ensure_parsers(excpt, ctx);
    hlt_list_push_back(_parsers, &hlt_type_info_struct_string_name_string_description_caddr_parse_func_caddr_resume_func, &parser, excpt, ctx);
}

void binpacintern_call_init_func(void (*func)(hlt_exception** excpt, hlt_execution_context* ctx))
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();
    
    (*func)(&excpt, ctx);
    
    if ( excpt )
        hlt_exception_print_uncaught(excpt, ctx);
}


