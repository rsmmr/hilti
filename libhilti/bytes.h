///
/// \addtogroup bytes
/// @{
/// Functions for manipulating bytes objects.
///
/// The main type is ~~hlt_bytes, which represents a HILTI bytes object.
///
/// A bytes object is internally represented as a list of variable-size data
/// chunks. While the list can be modified (e.g., adding another chunk of
/// data will append an entry to the list), the data chunks themselves are
/// immutable and can therefore be shared across multiple bytes objects. For
/// example, creating a new bytes object containing only a subsequence of an
/// existing one needs to create only a new list pointing to the appropiate
/// data.
///
/// There is one optimization of this basic scheme: To avoid fragmentation
/// when adding many tiny data chunks, in certain cases new data is copied
/// into an existing chunk. This will however never change any content a
/// chunk already has (because other bytes objects might use that as well)
/// but only *append* to a chunk's data if the initial allocation was large
/// enough. To avoid conflicts, only the bytes object which allocated a chunk
/// in the first place is allowed to extend it in this way; that object is
/// called the chunk's "owner".
///
/// To allow for efficient indexing and iteration over a bytes objects, there
/// are also position objects, ~~hlt_iterator_bytes. Each position is associated
/// with a particular bytes objects and can be used to locate a specific
/// byte. Creating a Position from an offset into the bytes can be
/// potentially expensive if the target position is far into the chunk list;
/// however once created, positions can be dererenced, incremented, and
/// decremented efficiently.

#ifndef LIBHILTI_bytes_H
#define LIBHILTI_bytes_H

#include "types.h"
#include "vector.h"
#include "rtti.h"
#include "int.h"

typedef int64_t hlt_bytes_size;     ///< Size of a ~~hlt_bytes instance, and also used for offsets.
typedef struct __hlt_bytes hlt_bytes; ///< Type for representing a HILTI ~~bytes object.
typedef struct __hlt_bytes_chunk __hlt_bytes_chunk;

/// A position with a ~~hlt_bytes instance.
typedef struct hlt_iterator_bytes {
    __hlt_bytes_chunk *chunk;  // Current chunk.
    int8_t* cur;               // Current position in chunk.

    // Note: Initially, there was a 3rd pointer, end, pointing to chunk->end
    // to allow for quicker checks whether the end of the block has been
    // removed. However, LLVM has trouble passing struct with three pointers
    // by value are return value, on Darwin at least. However, even if it
    // could, it seems that the Darwin ABI specifies only two registers for
    // return values and so a larger struct would require memory allocation;
    // that's probably not worth the gain we get by having chunk->end
    // available directly.
} hlt_iterator_bytes;

/// A pair of bytes instances.
typedef struct {
    hlt_bytes* first;  /// First element.
    hlt_bytes* second; /// Second element.
} hlt_bytes_pair;

/// Instantiates a new bytes object. The bytes object is initially empty.
///
/// Returns: The new bytes object.
extern hlt_bytes* hlt_bytes_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Instantiates a new bytes object from raw data. After instantiation, the
/// memory containing the raw data must not be modified or freed as the bytes
/// object might share it.
///
/// data: Pointer to the raw bytes.  The function takes ownership; it must
/// have been allocated with hlt_malloc().
///
/// len: Number of raw byes starting at *data*.
///
/// \hlt_c
///
/// Returns: The new bytes object.
extern hlt_bytes* hlt_bytes_new_from_data(int8_t* data, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Like hlt_new_bytes_from_data(), but does not take ownership of data.
///
/// data: Pointer to the raw bytes. The function does not take ownership.
///
/// len: Number of raw byes starting at *data*.
///
/// \hlt_c
///
/// Returns: The new bytes object.
extern hlt_bytes* hlt_bytes_new_from_data_copy(const int8_t* data, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the number of individual bytes stored in a bytes object.
///
/// b: The bytes object.
///
/// \hlt_c
///
/// Returns: The number of bytes stored in *b*.
///
/// Note: Calculating the length can potentially be expensive; it's not O(1).
extern hlt_bytes_size hlt_bytes_len(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Tests whether a bytes object is empty.
///
/// b: The bytes object.
///
/// \hlt_c
///
/// Note: This test is more efficient than comparing the result of
/// ~~hlt_bytes_len() to zero.
///
/// Returns: True if the object is empty.
extern int8_t hlt_bytes_empty(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Compares the content of two bytes objects.
///
/// b1: The first byte object.
/// b2: The second byte object.
///
/// \hlt_c
///
/// Returns: 0 if the two are equal; -1 if *b1* is lexicographically smaller
/// than *b2*, and 1 if *b2* is lexicographically smaller than *b1*.
extern int8_t hlt_bytes_cmp(const hlt_bytes* b1, const hlt_bytes* b2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Appends the contents of a bytes object to another. The two objects must not be the same.
///
/// b: The bytes object to append to.
/// other: The bytes object to append from.
/// \hlt_c
///
/// Raises: ValueError - If the two objects are the same, or *b* has been
/// frozen.
extern void hlt_bytes_append(hlt_bytes* b, const hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_bytes* hlt_bytes_concat(hlt_bytes* b1, const hlt_bytes* b2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Appends a sequence of raw bytes in memory to a bytes object. After
/// appending, the memory by the raw bytes must not be modified or freed as
/// the bytes object might share it.
///
/// b: The bytes object to append to.
///
/// raw: A pointer to the beginning of the byte sequence to append. The
/// function takes ownership; it must have been allocated with hlt_malloc().
///
/// len: The number of bytes to append starting from *raw*. \hlt_c
///
/// Raises: ValueError - If *b* has been frozen.
extern void hlt_bytes_append_raw(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Appends a sequence of raw bytes in memory to a bytes object. This
/// function does not take ownership of the appended data, but makes a copy
/// internally.
///
/// b: The bytes object to append to.
///
/// raw: A pointer to the beginning of the byte sequence to append.
///
/// len: The number of bytes to append starting from *raw*. \hlt_c
///
/// Raises: ValueError - If *b* has been frozen.
extern void hlt_bytes_append_raw_copy(hlt_bytes* b, int8_t* raw, hlt_bytes_size len, hlt_exception** excpt, hlt_execution_context* ctx);

/// Searches for the first occurance of a specific byte in a bytes object. 
///
/// b: The bytes object to search.
/// chr: The byte to search.
/// \hlt_c
///
/// Returns: The position where the byte is found, or ~~hlt_bytes_end if not found.
extern hlt_iterator_bytes hlt_bytes_find_byte(hlt_bytes* b, int8_t chr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Checks if a another bytes object is subsequence of a bytes object.
///
/// b: The bytes object to search.
/// other: The other bytes to search for.
/// \hlt_c
///
/// Returns: True if *other* was found *b*.
extern int8_t hlt_bytes_contains_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx);

/// Searches for the first occurance of another bytes object in a bytes object.
///
/// b: The bytes object to search.
/// other: The other bytes to search.
/// \hlt_c
///
/// Returns: The position where bytes is found, or ~~hlt_bytes_end if not found.
extern hlt_iterator_bytes hlt_bytes_find_bytes(hlt_bytes* b, hlt_bytes* other, hlt_exception** excpt, hlt_execution_context* ctx);

/// Matches a bytes objects against the sequence started by an interator. 
///
/// pos: The position where to match *b* againt.
/// b: The bytes to search.
/// \hlt_c
///
/// Returns: True if bytes a position *pos* start with b.
extern int8_t hlt_bytes_match_at(hlt_iterator_bytes pos, hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a subsequence of a bytes object.
///
/// start: The start of the subsequence.
/// end: The end of the subsequence; *end* itself is not included anymore.
/// \hlt_c
///
/// Returns: A new bytes object representing the subsequence.
///
/// Raises: ValueError - If any of the positions is found to be out of range.
extern hlt_bytes* hlt_bytes_sub(hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a subsequence of a bytes object as a raw C array.
///
/// start: The start of the subsequence.
/// end: The end of the subsequence; *end* itself is not included anymore.
/// \hlt_c
///
/// Returns: A pointer to continuous memory containing the subsequence.
/// Passed ownership, the memory must be freed with hlt_free(). Only
/// ``hlt_iterator_bytes_diff(start, end)`` number of bytes are valid. In
/// particular, if *start* equals *end* no byte must be read from the
/// returned pointer.
///
/// Raises: ValueError - If any position is found to be out of range.
extern int8_t* hlt_bytes_sub_raw(hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Copies a byte object into a new instance.
///
/// b: The bytes object to duplicate.
///
/// \hlt_c
///
/// Returns: The new object.
extern hlt_bytes* hlt_bytes_copy(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a bytes object into a raw C array.
///
/// b: The object to convert.
/// \hlt_c
///
/// Returns: A pointer to continuous memory containing the bytes.  Passed
/// ownership, the memory must be freed with hlt_free(). Only
/// ``hlt_bytes_len(b)`` number of bytes are valid.
extern int8_t* hlt_bytes_to_raw(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a bytes object into a raw C buffer provided by the user.
///
/// b: The object to convert.
///
/// buffer: The buffer to copy the raw data into.
///
/// buffer_size: The number of bytes available in \a buffer. If not
/// sufficient, no more will be written into it.
///
/// \hlt_c
///
/// Returns: A pointer to one beyond the last byte written, or null if the
/// buffer did not have sufficient size (in which case it will still have
/// been filled upto its mac.).
extern int8_t* hlt_bytes_to_raw_buffer(const hlt_bytes* b, int8_t* buffer, hlt_bytes_size buffer_size, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns one byte from a bytes object.
///
/// pos: The position from where to extract the byte. After reading the
/// byte, the iterator is incremented to the next position, or
/// hlt_bytes_end() if the end has been reached. If *pos* is already the
/// *end* position (or the end of the bytes object), it is *dtored* and
/// a ~~ValueError exception is raised.
///
/// end: End position.
///
/// \hlt_c
///
/// Raises: WouldBlock - If *pos* is already the end of bytes object.
///
/// Note: This function is optimized for the case where you need a few
/// subsequent bytes from the string as well an iterator pointing to the
/// remainder of the bytes object. That's what is common with ``unpack``
/// implementations.
///
/// Todo: Once we start compiling libhilti with llvm-gcc, calls to this function
/// should be optimized away. Check that.
extern int8_t __hlt_bytes_extract_one(hlt_iterator_bytes* pos, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// Creates a new position object representing a specific offset.
///
/// b: The bytes object to create the position for.
/// \hlt_c
///
/// offset: The offset within the object the position is to represent. The
/// offset is zero-based and must not extend beyond the length of the
/// stringth. If offset is negative, the length of the bytes object will be
/// added to it; in other words, negative offsets count from the end.
///
/// Note: Determing the length can be expensive; it's not O(1).
///
/// Raises: ValueError - If *offset* is found to be out of range.
extern hlt_iterator_bytes hlt_bytes_offset(const hlt_bytes* b, hlt_bytes_size offset, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a position representing the first element of a bytes object.
///
/// b: The bytes object to return the beginning of.
/// \hlt_c
///
/// Returns: The position.
extern hlt_iterator_bytes hlt_bytes_begin(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a position representing the end of any bytes object.
///
/// b: The bytes object to return the beginning of.
/// \hlt_c
///
/// Returns: The position.
extern hlt_iterator_bytes hlt_bytes_end(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a position representing the end of *any* bytes object.
///
/// Returns: The position.
///
/// Note: This function returns a end position that will match the end
/// position of any bytes object. However, it is not attached to any specific
/// one so it can't go beyond a former end position if more data is appended.
extern hlt_iterator_bytes hlt_bytes_generic_end(hlt_exception** excpt, hlt_execution_context* ctx);

/// Freezes/unfreezes a bytes object. A frozen object cannot be further
/// extended via any of the append functions. If the object already is in the
/// given state, the call is ignored. Note that empty bytes objects cannot be
/// frozen (unfreezing them is fine).
///
/// b: The bytes object to (un-)freeze.
///
/// freeze: 1 to freeze, 0 to unfreeze.
///
/// \hlt_c
///
/// Raises: ValueError - If the bytes object is empty.
extern void hlt_bytes_freeze(hlt_bytes* b, int8_t freeze, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns whether a bytes object has been frozen. For an empty object, this
/// returns always 0.
///
/// b: The bytes object.
///
/// \hlt_c
///
/// Returns: 1 if frozen, 0 otherwise..
extern int8_t hlt_bytes_is_frozen(const hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// Trims a bytes object at the beginning by removing data up to a given
/// position. The iterator will remain valid afterwards and still point to the
/// same location, which will now be the first of the bytes object.
///
///
/// b: The bytes object to be trimmed.
///
/// pos: The position up to which to trim. Afterwards, the bytes object will start with the bytes located by *pos*.
///
/// \hlt_c
///
/// Note that the result is undefined if the given iterator does actually not refer to a location inside the
/// bytes object.
void hlt_bytes_trim(hlt_bytes* b, hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns whether the bytes object a position is refering to has been
/// frozen. For an empty bytes object as well as for the generic end
/// position, this returns always 0.
///
/// pos: The position.
///
/// \hlt_c
///
/// Returns: 1 if frozen, 0 otherwise..
extern int8_t hlt_iterator_bytes_is_frozen(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx);

/// Creates a copy of a bytes iterator. The copy will point to the same location.
///
/// pos: The position to copy.
/// \hlt_c
///
/// Returns: The copied iterator.
extern hlt_iterator_bytes hlt_iterator_bytes_copy(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx);

/// Extracts the element at a position.
///
/// pos: The position to dereference.
/// \hlt_c
///
/// Returns: The element.
///
/// Raises: ValueError - If *pos* is found to be out of range.
extern int8_t hlt_iterator_bytes_deref(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx);

/// Increases a position by one. If the given position is already the last
/// one, the increased position will return True if compated with
/// ~~hlt_iterator_bytes_end() via ~~hlt_iterator_bytes_eq(). If one tries to
/// further increase such a position, it will be left untouched.
///
/// pos: The position to increase.
/// \hlt_c
extern hlt_iterator_bytes hlt_iterator_bytes_incr(hlt_iterator_bytes pos, hlt_exception** excpt, hlt_execution_context* ctx);

/// Increases a position by a given number of positions. If this exceeds the
/// number of available bytes, the return position will return True if
/// compared with ~~hlt_iterator_bytes_end() via ~~hlt_iterator_bytes_eq().
///
/// pos: The position to increase.
/// n: The number of bytes to skip.
/// \hlt_c
extern hlt_iterator_bytes hlt_iterator_bytes_incr_by(hlt_iterator_bytes pos, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx);

/// Compares two positions whether they refer to the same offset within a bytes object.
///
/// pos1: The first position to be compared.
/// pos2: The second position to be compares.
/// \hlt_c
///
/// Returns: True if the position refer to the same location. Returns false in
/// particular when *pos1* and *pos2* do not refer to the same bytes object.
extern int8_t hlt_iterator_bytes_eq(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Calculates the number of bytes between two position. Both positions must
/// refer to the same bytes object.
///
/// pos1: The starting position.
/// pos2: The end position; the byte pointed to by *pos* is not counted anymore. *end* must be >= *start*.
/// \hlt_c
///
/// Returns: The number of bytes between *pos1* and *pos2*.
///
/// Raises: ValueError - If any of the positions is found to be out of range.
extern hlt_bytes_size hlt_iterator_bytes_diff(hlt_iterator_bytes pos1, hlt_iterator_bytes pos2, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a bytes object into a string object.
///
/// Include: include-to-string-sig.txt
///
/// Note: The conversion can be expensive.
hlt_string hlt_bytes_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

///< A block used for iteration by ~~hlt_bytes_iterate_raw.
struct hlt_bytes_block {
    const int8_t* start; ///< Pointer to first data byte of block
    const int8_t* end;   ///< Pointer to one beyond last data byte of block.

    __hlt_bytes_chunk* next; // Pointer to next chunk.
    };

typedef struct hlt_bytes_block hlt_bytes_block;

/// Iterates through chunks of the raw data in usually large but not further specified amounts.
///
/// block: The field *start* and *end* will be set to the first and last bytes
/// of the next piece of data, respectively (they may be same for a
/// zero-lenght block of data). Note that a ~~hlt_bytes_block may contain
/// further fields which the function may use internally. You must pass in the
/// same instance on all calls of a single iteration.
///
/// cookie: Internal cookie for keeping track of the iteration. For the first
/// call, this must be NULL. Subsequently, always pass in the value returned
/// by the previous call.
///
/// start: The start of the subsequence to iterate through. Pass in the same
/// value on all calls of a single iteration.
///
/// end: The end of the subsequence; *end* itself is not included anymore.
/// Pass in the same value on all calls of a single iteration.
///
/// \hlt_c
///
/// Returns: Cookie for next call, or NULL if end has been reached. In the
/// latter case, block will still contain the final data (which may have a
/// length of zero); don't call the function again then.
extern void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_iterator_bytes start, hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
int64_t hlt_bytes_to_int(hlt_bytes* b, int64_t base, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
int64_t hlt_bytes_to_int_binary(hlt_bytes* data, hlt_enum byte_order, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_bytes* hlt_bytes_lower(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_bytes* hlt_bytes_upper(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern int8_t hlt_bytes_starts_with(hlt_bytes* b, hlt_bytes* s, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_bytes_pair hlt_bytes_split1(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_vector* hlt_bytes_split(hlt_bytes* b, hlt_bytes* sep, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_bytes* hlt_bytes_strip(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX

/// @}

#endif
