//
// Runtime functions supporting the generated HILTI/BinPAC++ code.
//
// These function all assume either "HILTI-C" linkage.

#ifndef BRO_PLUGIN_HILTI_RUNTIME_H
#define BRO_PLUGIN_HILTI_RUNTIME_H

extern "C" {

#include <libhilti/libhilti.h>

// Returns the ConnVal corresponding to the connection currently being
// analyzed. The cookie is a pointer to a Pac2Analyzer::Cookie instance.
void* libbro_cookie_to_conn_val(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns a boolean value corresponding whether we're currently parsing the
// originator side of a connection. The cookie is a pointer to a
// Pac2Analyzer::Cookie instance.
void* libbro_cookie_to_is_orig(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a HILTI bytes value into a Bro StringVal.
void* libbro_h2b_bytes(hlt_bytes* value, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a signed HILTI integer value into a Bro Val.
void* libbro_h2b_integer_signed(int64_t i, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts an unsigned HILTI integer value into a Bro Val.
void* libbro_h2b_integer_unsigned(uint64_t i, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a HILTI address value into a Bro AddrVal.
void* libbro_h2b_address(hlt_addr a, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a HILTI string value into a Bro StringVal.
void* libbro_h2b_string(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a HILTI double value into a Bro Val.
void* libbro_h2b_double(double d, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a HILTI double value into a Bro Val.
void* libbro_h2b_bool(int8_t d, hlt_exception** excpt, hlt_execution_context* ctx);

// Raises a given Bro events. The arguments are given as a tuple of Bro Val
// instances. The function takes ownership of those instances.
void libbro_raise_event(hlt_bytes* name, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_begin(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_set_size(uint64_t size, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_data_in(hlt_bytes* data, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_data_in_at_offset(hlt_bytes* data, uint64_t offset, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_gap(uint64_t offset, uint64_t len, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_file_end(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_dpd_confirm(void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

// XXX
void bro_rule_match(hlt_enum pattern_type, hlt_bytes* data, int8_t bol, int8_t eol, int8_t clear, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);
}

#endif
