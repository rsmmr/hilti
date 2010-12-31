// $Id
//
// BinPAC++ API for host applications.

#ifndef BINPAC_H
#define BINPAC_H

#include <hilti.h>

/// The main entry function to a BinPAC-generated parser. This function is
/// used for both starting the parsing initially and resuming it after a
/// YieldException. The protocols is as follows: TODO
typedef void* binpac_parse_function(hlt_bytes_pos iter, int8_t reserved, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void* binpac_resume_function(hlt_exception* yield, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions for parsing from sink.write. Here, we already have the
// parsing object.
typedef void* _binpac_parse_sink_function(void* pobj, hlt_bytes_pos iter, int8_t reserved, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void* _binpac_resume_sink_function(hlt_exception* yield, hlt_exception** excpt, hlt_execution_context* ctx);

// Predefined exceptions.

/// Raised when a generated parser encounters an error in its input.
extern hlt_exception binpac_parseerror;

/// Structure defining an BinPAC generated parser.
///
/// Todo: hiltic should generate the prototype for this struct.
typedef struct {
    int32_t internal;                    // HILTI-internal.
    hlt_string name;                     /// Short descriptive name.
    hlt_string description;              /// Longer textual description.
    hlt_list* mime_types;                /// list<string> of all MIME types handled by this parser.
    binpac_parse_function* parse_func;   /// The C function performing the parsing.
    binpac_resume_function* resume_func; /// The C function resuming parsing after a yield.
    _binpac_parse_sink_function* _parse_func_sink;   // The C function performing the parsing for sink.write. For internal use only. 
    _binpac_resume_sink_function* _resume_func_sink; // The C function resuming sink parsing after a yield. For internal use only. 
} binpac_parser;

/// Must be called exactly once at program startup to initialize the BinPAC
/// runtime.
extern void binpac_init();

/// Returns a list of all BinPAC generated parsers available.
///
/// Returns: List of binpac_parser* instances - The available parsers.
///
/// excpt: &
extern hlt_list* binpac_parsers(hlt_exception** excpt, hlt_execution_context* ctx);

/// Enables debugging output compiled into the BinPAC++ parsers.
///
/// enabled: 1 to enable, 0 to disable.
///
/// Note: The outut enabled here is only compiled in if binpac++ is run with ``-d``.
extern void binpac_enable_debugging(int8_t enabled);

/// Returns whether debugging outout compiled into BinPAC++ parser is enabled.
///
/// Returns: 1 if enabled, 0 otherwise.
extern int8_t binpac_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx);

#endif
