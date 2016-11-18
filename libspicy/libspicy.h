///
/// Spicy API for host applications.
///

#ifndef LIBSPICY_SPICY_H
#define LIBSPICY_SPICY_H

#include <libhilti/libhilti.h>

#include "libspicy/autogen/spicy-hlt.h"
#include "libspicy/autogen/spicyhilti-hlt.h"

#include "init.h"
#include "rtti.h"

/// The main entry function to a Spicy-generated parser.
typedef void* spicy_parse_function(hlt_bytes* data, void* user, hlt_exception** excpt,
                                   hlt_execution_context* ctx);
typedef void* spicy_resume_function(hlt_exception* yield, hlt_exception** excpt,
                                    hlt_execution_context* ctx);
typedef void spicy_compose_output_function(hlt_bytes* data, void** obj, hlt_type_info* type,
                                           void* user, hlt_exception** excpt,
                                           hlt_execution_context* ctx);
typedef void spicy_compose_function(void* pobj, spicy_compose_output_function* output, void* user,
                                    hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions for parsing from sink.write.
typedef void __spicy_parse_sink_function(void* pobj, hlt_bytes* data, void* user,
                                         hlt_exception** excpt, hlt_execution_context* ctx);
typedef void __spicy_resume_sink_function(hlt_exception* yield, hlt_exception** excpt,
                                          hlt_execution_context* ctx);

// Internal function to create an instance of a parser.
struct spicy_sink;
typedef void* __spicy_new_function(struct spicy_sink* sink, hlt_bytes* mimetype, int8_t try_mode,
                                   void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions to trigger specific hooks.
typedef void __spicy_gap_hook(void* pobj, void* cookie, uint64_t seq, uint64_t len,
                              hlt_exception** excpt, hlt_execution_context* ctx);
typedef void __spicy_skip_hook(void* pobj, void* cookie, uint64_t seq, hlt_exception** excpt,
                               hlt_execution_context* ctx);
typedef void __spicy_overlap_hook(void* pobj, void* cookie, uint64_t seq, hlt_bytes* b1,
                                  hlt_bytes* b2, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void __spicy_undelivered_hook(void* pobj, void* cookie, uint64_t seq, hlt_bytes* b,
                                      hlt_exception** excpt, hlt_execution_context* ctx);

/// Structure defining an Spicy generated parser.
///
/// TODO: hiltic should generate the prototype for this struct.
typedef struct __spicy_parser {
    __hlt_gchdr __gchdr;    // HILTI-internal.
    int32_t internal;       // HILTI-internal.
    hlt_string name;        /// Short descriptive name.
    hlt_string description; /// Longer textual description.
    hlt_list* ports;        /// List of well-known ports associated with parser.
    int32_t params; /// Number of additional (type) parameters that the parse functions receive.
    hlt_list* mime_types;                 /// list<string> of all MIME types handled by this parser.
    spicy_parse_function* parse_func;     /// The C function performing the parsing.
    spicy_resume_function* resume_func;   /// The C function resuming parsing after a yield.
    spicy_compose_function* compose_func; /// The C function composing data to binary.
    hlt_type_info* type_info;             /// Type information for the parsed struct.

    __spicy_parse_sink_function* parse_func_sink;   // The C function performing the parsing for
                                                    // sink.write. For internal use only.
    __spicy_resume_sink_function* resume_func_sink; // The C function resuming sink parsing after a
                                                    // yield. For internal use only.
    __spicy_new_function* new_func; // C function to create a new instance of the parser. May be
                                    // None of not supported.
    // Note that this function must not take any further parser parameters.

    __spicy_gap_hook* hook_gap;   // Hook to signal a gap in the input stream.
    __spicy_skip_hook* hook_skip; // Hook to signal explicitly skipping parts of the input stream.
    __spicy_overlap_hook* hook_overlap; // Hook to report an ambigious overlap in data chunks.
    __spicy_undelivered_hook*
        hook_undelivered; // Hook to report an chunk that could not be put into sequence.

} spicy_parser;

/// Returns a list of all Spicy generated parsers available.
///
/// Returns: List of spicy_parser* instances - The available parsers.
///
/// excpt: &
extern hlt_list* spicy_parsers(hlt_exception** excpt, hlt_execution_context* ctx);

/// Internal function called from spicy_init() to install parsers registered
/// already at that time.
void __spicy_register_pending_parsers();

/// Enables debugging output compiled into the Spicy parsers.
///
/// enabled: 1 to enable, 0 to disable.
///
/// Note: The output enabled here is only compiled in the Spicy compiler
/// runs with a debug level > 0.
extern void spicy_enable_debugging(int8_t enabled);

/// Returns whether debugging outout compiled into Spicy parser is
/// enabled.
///
/// Returns: 1 if enabled, 0 otherwise.
extern int8_t spicy_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx);

// Internal wrapper around spicy_debugging_enabled() to make it accessible
// from the SpicyHilti namespace.
extern int8_t spicyhilti_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx);


#endif
