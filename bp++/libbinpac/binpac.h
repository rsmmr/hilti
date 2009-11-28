// $Id
// 
// BinPAC++ API for host applications.

#include <hilti.h>

/// The main entry function to a BinPAC generated parser. This function is
/// used for both starting the parsing initially and resuming it after a
/// YieldException. The protocols is as follows: TODO
typedef void* binpac_parse_function(hlt_bytes_pos iter, hlt_exception** excpt);
typedef void* binpac_resume_function(hlt_exception* yield, hlt_exception** excpt);

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
    binpac_parse_function *parse_func;   /// The C function performing the parsing.
    binpac_resume_function *resume_func; /// The C function resuming parsing after a yield.
} binpac_parser;

/// Must be called exactly once at program startup to initialize the BinPAC
/// runtime.
void binpac_init();

/// Returns a list of all BinPAC generated parsers available. 
/// 
/// Returns: List of binpac_parser* instances - The available parsers.
/// 
/// excpt: &
hlt_list* binpac_parsers(hlt_exception** excpt);
