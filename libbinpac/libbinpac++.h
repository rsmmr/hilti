///
/// BinPAC++ API for host applications.
///

#ifndef LIBBINPAC_BINPAC_H
#define LIBBINPAC_BINPAC_H

#include <libhilti.h>

#include "autogen/binpac-hlt.h"

/// The main entry function to a BinPAC-generated parser.
typedef void* binpac_parse_function(hlt_bytes* data, void* user, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void* binpac_resume_function(hlt_exception* yield, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions for parsing from sink.write.
typedef void* __binpac_parse_sink_function(void* pobj, hlt_bytes* data, int8_t reserved, void* user, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void* __binpac_resume_sink_function(hlt_exception* yield, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal function to create an instance of a parser.
struct binpac_sink;
typedef void* __binpac_new_function(struct binpac_sink* sink, hlt_bytes* mimetype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Structure defining an BinPAC generated parser.
///
/// TODO: hiltic should generate the prototype for this struct.
typedef struct {
    __hlt_gchdr __gchdr;                 // HILTI-internal.
    int32_t internal;                    // HILTI-internal.
    hlt_string name;                     /// Short descriptive name.
    hlt_string description;              /// Longer textual description.
    int32_t params;                      /// Number of additional (type) parameters that the parse functions receive.
    hlt_list* mime_types;                /// list<string> of all MIME types handled by this parser.
    binpac_parse_function* parse_func;   /// The C function performing the parsing.
    binpac_resume_function* resume_func; /// The C function resuming parsing after a yield.
    hlt_type_info* type_info;            /// Type information for the parsed struct.

    __binpac_parse_sink_function* parse_func_sink;   // The C function performing the parsing for sink.write. For internal use only. 
    __binpac_resume_sink_function* resume_func_sink; // The C function resuming sink parsing after a yield. For internal use only.
    __binpac_new_function* new_func;                 // C function to create a new instance of the parser. May be None of not supported.
                                                     // Note that this function must not take any further parser parameters.
} binpac_parser;

/// Must be called exactly once at program startup to initialize the BinPAC
/// runtime.
extern void binpac_init();

/// Terminate the binpac runtime. If not called explicitly, it will be
/// automatically run at termination.
extern void binpac_done();

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
/// Note: The output enabled here is only compiled in the BinPAC compiler
/// runs with a debug level > 0.
extern void binpac_enable_debugging(int8_t enabled);

/// Returns whether debugging outout compiled into BinPAC++ parser is
/// enabled.
///
/// Returns: 1 if enabled, 0 otherwise.
extern int8_t binpac_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx);

#endif
