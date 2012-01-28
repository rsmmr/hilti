///
/// \addtogroup string
/// @{
/// Functions for manipulating string objects.
///
/// A HILTI string is represented by an instance of \ref hlt_string.
/// Internally, they are represented with UTF-8 encoding, which means that
/// their logical length may differ from the numbers of bytes needed for
/// their storage. String objects should only be manipulated via the provided
/// library function.
///
/// A \ref hlt_string (and thus a string) is copied by value, which is
/// efficient as internally, a\ ref hlt_string is just a pointer to the
/// actual object. This works because HILTI strings are not mutable. However,
/// one must still use \ref hlt_string_ref / \ht_string_unref to
/// increase/decrease reference counts as copies are created/deleted.
///
/// Note that an empty string is represented by a nullptr. The library
/// functions all handle a nullptr accordingly.

#ifndef HILTI_STRING_H
#define HILTI_STRING_H

#include <stdio.h>

#include "types.h"
#include "memory.h"

/// Type for the size of, and offsets into, a string.
typedef int64_t hlt_string_size;

struct hlt_bytes;

/// The internal representation of a HILTI string object. Note that \ref
/// hlt_string is a \a pointer to an instance of this struct.
struct __hlt_string {
    __hlt_gchdr __gchdr;  // Header for memory management.
    hlt_string_size len;  // Length of byte array. This is *not* the logical length of the string
                          // as it does not accoutn for character encoding.
    int8_t bytes[];       // The bytes representing the strings value, encoded in UTF-8.
} __attribute__((__packed__));

/// \hlt_ref
extern void hlt_string_ref(hlt_string s);

/// \hlt_unref
extern void hlt_string_unref(hlt_string s);

/// Returns a given string directly back. This is a wrapper function that has
/// the right signature to use in an \ref hlt_type_info object.
///
/// \hlt_to_string
extern hlt_string hlt_string_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the length of a string. This is the logical length that accounts
/// for character encoding, not the number of bytes the strings needs for
/// storage.
///
/// s: The string.
///
/// \hlt_c
extern hlt_string_size hlt_string_len(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// Concatenates two strings and returns the result.
///
/// s1: The first string.
///
/// s2: The second string.
///
/// \hlt_c
///
/// Returns: The concatenation.
extern hlt_string hlt_string_concat(hlt_string s1, hlt_string s2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Extracts a substring from a string.
///
/// s: The string to extract from.
///
/// pos: The starting position from where to extract, counting from zero.
///
/// len: The number of characters to extract, counting from \c pos.
///
/// \hlt_c
///
/// Returns: The extracted substring.
extern hlt_string hlt_string_substr(hlt_string s, hlt_string_size pos, hlt_string_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Searches a string in another.
///
/// s: The string to search in.
///
/// pattern: The string search for.
///
/// \hlt_c
///
/// Returns the first position where \c pattern is found in \s (counting from
/// zero), or -1 if not found.
extern hlt_string_size hlt_string_find(hlt_string s, hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx);

/// Compares two strings. This does a lexicographical comparision based on
/// UTF-8 characters.
///
/// s1: The first string.
///
/// s2: The second string.
///
/// \hlt_c
///
/// Returns: -1 if \c s1 is smaller than \c s2; 0 if \c s1 is equal to \c s2;
/// and 1 if \c s1 is larger than \c s2;
///
/// \todo This should take the current localte into account (or should it not?).
extern int8_t hlt_string_cmp(hlt_string s1, hlt_string s2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Encodes a string into a raw represention using a given character set.
///
/// s: The string to encode.
///
/// charset: The charset to encode in. Currently, only \c utf8 and \c ascii are supported.
///
/// \hlt_c
///
/// Returns: The encoded raw representation.
///
/// \todo: This should supoprt more character sets.
extern struct __hlt_bytes* hlt_string_encode(hlt_string s, hlt_string charset, hlt_exception** excpt, hlt_execution_context* ctx);

/// Decodes the raw representation of a string in a given character set into
/// a string object.
///
/// raw: The raw representation, encoded in character set \c charset.
///
/// charset: The character set \c raw is encoded in. Currently, only \c utf8 and \c ascii are supported.
///
/// Returns: The decoded string.
///
/// \todo: This should supoprt more character sets.
extern hlt_string hlt_string_decode(struct __hlt_bytes* raw, hlt_string charset, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the empty string.
///
/// \hlt_c
///
/// Returns: The empty string.
extern hlt_string hlt_string_empty(hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new string from a C ASCIIZ string. The ASCIIZ string is
/// assumed to be encoded in UTF8.
///
/// asciiz: The C string.
///
/// \hlt_c
///
/// Returns: The new HILTI string.
extern hlt_string hlt_string_from_asciiz(const char* asciiz, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new string from a C byte array. The bytes are assumed to
/// be encoded in UTF8.
///
/// data: A pointer to the bytes.
///
/// len: The number of bytes to use, starting at \c data.
///
/// \hlt_c
///
/// Returns: The new HILTI string.
extern hlt_string hlt_string_from_data(const int8_t* data, hlt_string_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a string representatin of an HILTI object.
///
/// type: Type information for \c obj.
///
/// obj: A \a pointer to the object to turn into a string representation.
///
/// \hlt_c
///
/// Returns: The string representation.
extern hlt_string hlt_string_from_object(const hlt_type_info* type, void* obj, hlt_exception** excpt, hlt_execution_context* ctx);

/// Prints a string to a file. This is mainly for debugging purposes.
///
/// file: The output file.
///
/// s: The string to print.
///
/// newline: True if a newline should be added after printing \c s.
///
/// \hlt_c
///
/// \todo As we don't handle locales yet, the function can only print ASCII
/// characters directly. It turns all non-ASCII characters into an escaped
/// representation.
extern void hlt_string_print(FILE* file, hlt_string s, int8_t newline, hlt_exception** excpt, hlt_execution_context* ctx);

/// Prints up to a given number of a string's characters to to a file. If the
/// string is longer than the given limit, the function adds \c ... to the
/// output. This functin is mainly for debugging purposes.
///
/// file: The output file.
///
/// s: The string to print.
///
/// newline: True if a newline should be added after printing \c s.
///
/// n: The maximum number of characters to print.
///
/// \hlt_c
///
/// \todo As we don't handle locales yet, the function can only print ASCII
/// characters directly. It turns all non-ASCII characters into an escaped
/// representation.
extern void hlt_string_print_n(FILE* file, hlt_string s, int8_t newline, hlt_string_size n, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a string into a native, null-terminated string.
///
/// s: The string to convert.
///
/// \hlt_c.
///
/// Returns: A pointer to the native string. THe function passes ownership to
/// the caller, who needs to call hlt_free() once done.
///
/// \todo "Native" is supposed mean the local character set, but we don't do
/// any character set conversion yet.
extern const char* hlt_string_to_native(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// @}

#endif
