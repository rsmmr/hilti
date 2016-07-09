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

#ifndef LIBHILTI_STRING_H
#define LIBHILTI_STRING_H

#include <stdio.h>

#include "enum.h"
#include "memory_.h"
#include "rtti.h"
#include "types.h"

/// Type for the size of, and offsets into, a string.
typedef int64_t hlt_string_size;

/// The internal representation of a HILTI string object. Note that \ref
/// hlt_string is a \a pointer to an instance of this struct.
struct __hlt_string {
    __hlt_gchdr __gchdr; // Header for memory management.
    hlt_string_size len; // Length of byte array. This is *not* the logical length of the string
                         // as it does not accoutn for character encoding.
    int8_t bytes[];      // The bytes representing the strings value, encoded in UTF-8.
} __attribute__((__packed__));

/// Returns a given string directly back. This is a wrapper function that has
/// the right signature to use in an \ref hlt_type_info object.
///
/// \hlt_to_string
extern hlt_string hlt_string_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                                       __hlt_pointer_stack* seen, hlt_exception** excpt,
                                       hlt_execution_context* ctx);

/// Returns the length of a string. This is the logical length that accounts
/// for character encoding, not the number of bytes the strings needs for
/// storage.
///
/// s: The string.
///
/// \hlt_c
extern hlt_string_size hlt_string_len(hlt_string s, hlt_exception** excpt,
                                      hlt_execution_context* ctx);

/// Concatenates two strings and returns the result.
///
/// s1: The first string.
///
/// s2: The second string.
///
/// \hlt_c
///
/// Returns: The concatenation.
extern hlt_string hlt_string_concat(hlt_string s1, hlt_string s2, hlt_exception** excpt,
                                    hlt_execution_context* ctx);

/// Concatenates a ASCIIZ string to a HILTI string.
///
/// s1: The first string.
///
/// s2: The second string in null-terminated UTF8.
///
/// \hlt_c
///
/// Returns: The concatenation.
extern hlt_string hlt_string_concat_asciiz(hlt_string s1, const char* s2, hlt_exception** excpt,
                                           hlt_execution_context* ctx);

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
extern hlt_string hlt_string_substr(hlt_string s, hlt_string_size pos, hlt_string_size len,
                                    hlt_exception** excpt, hlt_execution_context* ctx);

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
extern hlt_string_size hlt_string_find(hlt_string s, hlt_string pattern, hlt_exception** excpt,
                                       hlt_execution_context* ctx);

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
extern int8_t hlt_string_cmp(hlt_string s1, hlt_string s2, hlt_exception** excpt,
                             hlt_execution_context* ctx);

/// Encodes a string into a raw represention using a given character set.
///
/// s: The string to encode.
///
/// charset: The charset to encode in. Currently, only ``Hilti_Charset_UTF8`` and
/// ``Hilti_Charset_ASCII`` are supported.
///
/// \hlt_c
///
/// Returns: The encoded raw representation.
///
/// \todo: This should supoprt more character sets.
extern struct __hlt_bytes* hlt_string_encode(hlt_string s, hlt_enum charset, hlt_exception** excpt,
                                             hlt_execution_context* ctx);

/// Decodes the raw representation of a string in a given character set into
/// a string object.
///
/// raw: The raw representation, encoded in character set \c charset.
///
/// charset: The character set \c raw is encoded in. Currently, ``Hilti_Charset_UTF8``,
/// ``Hilti_Charset_UTF16LE``, ``Hilti_Charset_UTF16BE``, ``Hilti_Charset_UTF32LE``,
/// ``Hilti_Charset_UTF32BE``, and ``Hilti_Charset_ASCII`` are supported.
///
/// Returns: The decoded string.
///
/// \todo: This should supoprt more character sets.
extern hlt_string hlt_string_decode(struct __hlt_bytes* raw, hlt_enum charset,
                                    hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the empty string.
///
/// \hlt_c
///
/// Returns: The empty string.
extern hlt_string hlt_string_empty(hlt_exception** excpt, hlt_execution_context* ctx);

/// Creates a copy of the string. Note that this returns a physically new
/// instance, which is rarely necessary because string's are immutable and
/// hence can be duplicated by creating a new reference to an existing
/// object.
///
/// s: The string to duplicate.
///
/// \hlt_c
///
/// Returns: The empty string.
extern hlt_string hlt_string_copy(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new string from a C ASCIIZ string. The ASCIIZ string is
/// assumed to be encoded in UTF8.
///
/// asciiz: The C string.
///
/// \hlt_c
///
/// Returns: The new HILTI string.
extern hlt_string hlt_string_from_asciiz(const char* asciiz, hlt_exception** excpt,
                                         hlt_execution_context* ctx);

/// Converts a string into a raw C array, using a UTF8 representation. It
/// will always add a null byte at the end. If the string itself contains
/// null bytes, the result is undefined.
///
/// dst: Buffer where to copy the raw C array to.
///
/// dst_len: Maximum number of bytes available in dst. If smaller than the
/// the size of the string object, the converted string will be truncated.
/// ///
///
/// \hlt_c
///
/// Returns: The length of converted string, excluding the final null byte.
extern int64_t hlt_string_to_asciiz(int8_t* dst, size_t dst_len, hlt_string s,
                                    hlt_exception** excpt, hlt_execution_context* ctx);

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
extern hlt_string hlt_string_from_data(const int8_t* data, hlt_string_size len,
                                       hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a string representatin of an HILTI object.
///
/// type: Type information for \c obj.
///
/// obj: A \a pointer to the object to turn into a string representation.
///
/// \hlt_c
///
/// Returns: The string representation.
extern hlt_string hlt_object_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                                       hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a string representatin of an HILTI object. This is an internal
/// version for recursive execution by conversion function; it takes a
/// pointer stack to avoid running into cycles.
///
/// type: Type information for \c obj.
///
/// obj: A \a pointer to the object to turn into a string representation.
///
/// seen: Stack of objects traversed so far.
///
/// \hlt_c
///
/// Returns: The string representation.
extern hlt_string __hlt_object_to_string(const hlt_type_info* type, const void* obj,
                                         int32_t options, __hlt_pointer_stack* seen,
                                         hlt_exception** excpt, hlt_execution_context* ctx);

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
extern void hlt_string_print(FILE* file, hlt_string s, int8_t newline, hlt_exception** excpt,
                             hlt_execution_context* ctx);

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
extern void hlt_string_print_n(FILE* file, hlt_string s, int8_t newline, hlt_string_size n,
                               hlt_exception** excpt, hlt_execution_context* ctx);

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
extern char* hlt_string_to_native(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_string hlt_string_join(hlt_string sep, hlt_list* l, hlt_exception** excpt,
                                  hlt_execution_context* ctx);


/// XXX
int8_t hlt_string_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2,
                        const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
hlt_hash hlt_string_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt,
                         hlt_execution_context* ctx);


/// @}

#endif
