/// \addtogroup regexp
/// @{

#ifndef LIBHILTI_REGEXP_H
#define LIBHILTI_REGEXP_H

#include "bytes.h"
#include "vector.h"
#include "list.h"

/// Type for regular expressions.
typedef struct __hlt_regexp hlt_regexp;

/// Type for state of token matching.
typedef struct __hlt_match_token_state hlt_match_token_state;

/// Pattern compilation flags.
typedef int64_t hlt_regexp_flags;
static const int HLT_REGEXP_NOSUB = 1;  /// Compile without support for capturing sub-expressions.

/// Type for range within a ~~Bytes object.
typedef struct {
    hlt_iterator_bytes begin;
    hlt_iterator_bytes end;
} hlt_regexp_range;

/// Type for the result of ~~hlt_regexp_bytes_span.
typedef struct {
    int32_t rc;
    hlt_regexp_range span;
} hlt_regexp_span;

/// Type for the result of ~~hlt_regexp_match_token.
typedef struct {
    int32_t rc;
    hlt_iterator_bytes end;
} hlt_regexp_match_token;

/// Instantiates a new Regexp instance.
///
/// flags: The compilation flags for the regexp.
///
/// excpt: &
///
/// Returns: The new Regexp instance.
extern hlt_regexp* hlt_regexp_new(hlt_regexp_flags flags, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new Regexp instance by recompiling the patterns from
/// another instance, potentially with new flags.
///
/// type: The type information for the new regexp type.
///
/// other: The old regexp to use the patterns from
///
/// excpt: &
///
/// Returns: The new Regexp instance.
extern hlt_regexp* hlt_regexp_new_from_regexp(hlt_regexp* other, hlt_exception** excpt, hlt_execution_context* ctx);

/// Compiles a pattern.
///
/// re: The regexp instance to compile the pattern into. An already compiled
/// regexp cannot be reused.
///
/// pattern: The pattern to compile.
///
/// excpt: &
///
/// Raises: ~~hlt_exception_pattern_error - If the pattern cannot be compiled;
/// Raises: ~~hlt_exception_value_error - If a pattern was already compiled into *re*.
extern void hlt_regexp_compile(hlt_regexp* re, hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx);

/// Compiles a set of patterns.
///
/// re: The regexp instance to compile the pattern into. An already compiled
/// regexp cannot be reused.
///
/// patterns: The list of patterns to compile.
///
/// excpt: &
///
/// Raises: ~~hlt_exception_pattern_error - If the pattern cannot be compiled;
/// Raises: ~~hlt_exception_value_error - If a pattern was already compiled into *re*.
extern void hlt_regexp_compile_set(hlt_regexp* re, hlt_list* patterns, hlt_exception** excpt, hlt_execution_context* ctx);

/// Searches a regexp within a ~~string.
///
/// re: The compiled pattern to search.
/// s: The string in which to search the pattern.
/// excpt: &
///
/// Returns: If the returned integer is larger than zero, the regexp was
/// found; for sets compiled via ~~hlt_compile_set the integer value then
/// indicates the index of the pattern that was found. If the function
/// returns zero, no match was found and that would not change if further
/// data would be added to the string *s*. If the returned value is smaller
/// than 0, a partial match was found (i.e., no match yet but adding further
/// data could change that).
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
///
/// Todo: This function is not yet implemented.
extern int32_t hlt_regexp_string_find(hlt_regexp* re, hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// Searches a pattern within a bytes.
///
/// re: The compiled pattern to search.
/// begin: The position where matching is to begin.
/// end: The position upto which matching is performed.
/// excpt: &
///
/// Returns: If the returned integer is larger than zero, the regexp was
/// found; for sets compiled via ~~hlt_compile_set the integer value then
/// indicates the index of the pattern that was found. If the function
/// returns zero, no match was found and that would not change if further
/// data would be added to the string *s*. If the returned value is smaller
/// than 0, a partial match was found (i.e., no match yet but adding further
/// data could change that).
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
extern int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the substring matching a regexp.
///
/// re: The compiled pattern to search.
/// s: The string in which to search the pattern.
/// excpt: &
///
/// Returns: The substring matching the regexp.
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
///
/// Todo: This function is not yet implemented.
extern hlt_string hlt_regexp_string_span(hlt_regexp* re, hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns byte range matching a regexp.
///
/// re: The compiled pattern to search.
/// begin: The position where matching is to begin.
/// end: The position upto which matching is performed.
/// excpt: &
///
/// Returns: The range matching the regexp; if no match is found, *begin* and
/// *end* are set to ~~hlt_bytes_end.
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
extern hlt_regexp_span hlt_regexp_bytes_span(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns substrings matching parenthesis expressions in a regexp.
///
/// re: The compiled pattern to search.
/// s: The string in which to search the pattern.
/// excpt: &
///
/// Returns: A vector of ~~hlt_string, containing all the substrings. The
/// index 0 corresponds to the whole expression, index 1 to the first
/// subexpression etc. If not match is found, the returned vector is empty.
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
///
/// Todo: This function is not yet implemented.
extern hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns byte ranges matching parenthesis expressions in a regexp.
///
/// re: The compiled pattern to search.
/// begin: The position where matching is to start.
/// end: The position upto which matching is performed.
/// excpt: &
///
/// Returns: A vector of ~~hlt_regexp_range, containing all the byte ranges.
/// The index 0 corresponds to the whole expression, index 1 to the first
/// subexpression etc. If not match is found, the returned vector is empty.
///
/// Raises: ~~hlt_exception_value_error - If no pattern has been compiled for *re* yet.
///
/// Todo: This function does not yet support sets as compiled via ~~hlt_regexp_compile_set.
extern hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a regexp into a string.
///
/// Include: include-to-string-sig.txt
hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a regexp into a null-terminated C string.
///
/// re: The regexp to render.
///
/// \hlt_c.
///
/// Returns: A pointer to the native string. THe function passes ownership to
/// the caller, who needs to call hlt_free() once done.
char* hlt_regexp_to_asciiz(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx);

/// TODO: Document.
extern hlt_regexp_match_token hlt_regexp_bytes_match_token(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// TODO: Document.
extern hlt_match_token_state* hlt_regexp_match_token_init(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx);

/// TODO: Document.
extern hlt_regexp_match_token hlt_regexp_bytes_match_token_advance(hlt_match_token_state* state, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// @}

#endif
