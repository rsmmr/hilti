/* $Id$
 * 
 * API for HILTI's bytes data type.
 * 
 */

#ifndef HILTI_BYTES_H
#define HILTI_BYTES_H

#include "hilti_intern.h"

typedef int32_t __hlt_bytes_size;
typedef struct __hlt_bytes __hlt_bytes;
typedef struct __hlt_bytes_chunk __hlt_bytes_chunk; // Private.

struct __hlt_bytes_pos {
    __hlt_bytes_chunk *chunk; // Current chunk.
    const int8_t* cur;        // Current position in chunk.
    
    // Note: Initially, there was a 3rd pointer, end, pointing to chunk->end
    // to allow for quicker checks whether the end of the block has been
    // removed. However, LLVM has trouble passing struct with three pointers
    // by value are return value, on Darwin at least. However, even if it
    // could, it seems that the Darwin ABI specifies only two registers for
    // return values and so a larger struct would require memory allocation;
    // that's probably not worth the gain we get by having chunk->end
    // available directly.
};

typedef struct __hlt_bytes_pos __hlt_bytes_pos;

// Instantiates a new Bytes object. The Bytes object is initially empty. 
// 
// Returns: The new Bytes object. 
extern __hlt_bytes* __hlt_bytes_new(__hlt_exception* excpt);

// Instantiates a new Bytes object from raw data. After instantiation, the
// memory containing the raw data must not be modified or freed as the Bytes
// object might share it. 
// 
// data: Pointer to the raw bytes.
// len: Number of raw byes starting at *data*.
//
// Returns: The new Bytes object. 
extern __hlt_bytes* __hlt_bytes_new_from_data(const int8_t* data, int32_t len, __hlt_exception* excpt);
 
// Returns: The Bytes object. 
extern __hlt_bytes* __hlt_bytes_new(__hlt_exception* excpt);

// Returns the number of individual bytes stored in a Bytes object.
// 
// Note: Calculating the length can potentially be expensive; it's not O(1).
// 
// b: The Bytes object. 
// 
// Returns: The number of bytes stored in *b.
extern __hlt_bytes_size __hlt_bytes_len(const __hlt_bytes* b, __hlt_exception* excpt);

// Tests whether a Bytes object is empty. 
// 
// Note: This test is more efficient than comparing the result of
// ~~__hlt_bytes_len() to zero. 
// 
// Returns: True if the object is empty.
extern int8_t __hlt_bytes_empty(const __hlt_bytes* b, __hlt_exception* excpt);

// Appends the contents of a Bytes object to another. The two objects must not be the same.
// 
// b: The Bytes object to append to.
// other: The Bytes object to append from.
// 
// Raises: ValueError - If the two objects are the same.
extern void  __hlt_bytes_append(__hlt_bytes* b, const __hlt_bytes* other, __hlt_exception* excpt);

// Appends a sequence of raw bytes in memory to a Bytes object. After
// appending, the memory by the raw bytes must not be modified or freed as
// the Bytes object might share it. 
// 
// b: The Bytes object to append to.
// raw: A pointer to the beginning of the byte sequence to append. 
// len: The number of bytes to append starting from *raw*.
extern void __hlt_bytes_append_raw(__hlt_bytes* b, const int8_t* raw, __hlt_bytes_size len, __hlt_exception* excpt);

// Returns a subsequence of a Bytes object. 
// 
// start: The start of the subsequence.
// end: The end of the subsequence; *end* itself is not included anymore.
// 
// Returns: A new Bytes object representing the subsequence.
// 
// Raises: ValueError - If any of the positions is found to be out of range.
extern __hlt_bytes* __hlt_bytes_sub(__hlt_bytes_pos start, __hlt_bytes_pos end, __hlt_exception* excpt);

// Returns a subsequence of a Bytes object as a raw C array. 
// 
// start: The start of the subsequence. 
// end: The end of the subsequence; *end* itself is not included anymore.
// 
// Returns: A pointer to continuous memory containing the subsequence. The
// memory must not be altered nor freed. Only ``__hlt_bytes_pos_diff(start,
// end)`` number of bytes are valid. In particular, if *start* equals *end*
// no byte must be read from the returned pointer.
//
// Raises: ValueError - If any position is found to be out of range.
//
// Note: This function can either be very efficient (if the substring is already located 
// in continous memory), or expensive (if it's not, as then it needs to be copied). 
extern const int8_t* __hlt_bytes_sub_raw(__hlt_bytes_pos start, __hlt_bytes_pos end, __hlt_exception* excpt);

// Returns one byte from a Bytes object.
//
// *pos*: The position from where to extract the byte. After reading the
// byte, the iterator is increment to the next position, or __hlt_bytes_end()
// if the end has been reached. If *pos* is already the *end* position (or
// the end of the bytes object), it is left unchanged and a ~~ValueError
// exception is raised.
// 
// *end*: End position. 
// 
// Raises: ValueError - If *pos* is already the end of bytes object. 
//
// Note: This function is optimized for the case where you need a few
// subsequent bytes from the string as well an iterator pointing to the
// remainder of the Bytes object. That's what is common with ``unpack``
// implementations. 
//
// Todo: Once we start compiling libhilti with llvm-gcc, calls to this function
// should be optimized away. Check that. 
extern int8_t __hlt_bytes_extract_one(__hlt_bytes_pos* pos, const __hlt_bytes_pos end, __hlt_exception* excpt);

// Creates a new position object representing a specific offset.
// 
// b: The Bytes object to create the position for.
// 
// offset: The offset within the object the position is to represent. The
// offset is zero-based and must not extend beyond the length of the
// stringth. If offset is negative, the length of the bytes object will be
// added to it; in other words, negative offsets count from the end. 
// 
// Note: Determing the length can be expensive; it's not O(1).
//
// Raises: ValueError - If *offset* is found to be out of range.
extern __hlt_bytes_pos __hlt_bytes_offset(const __hlt_bytes* b, __hlt_bytes_size offset, __hlt_exception* excpt);
    
// Returns a position representing the first element of a Bytes object.    
//
// b: The Bytes object to return the beginning of.
// 
// Returns: The position.
extern __hlt_bytes_pos __hlt_bytes_begin(const __hlt_bytes* b, __hlt_exception* excpt);

// Returns a position representing the one beyond the last element of a Bytes object.
// 
// b: The Bytes object to return the end of.
//
// Returns: The position.
extern __hlt_bytes_pos __hlt_bytes_end(const __hlt_bytes* b, __hlt_exception* excpt);

// Extracts the element at a position.  
// 
// pos: The position to dereference.
// 
// Returns: The element. 
// 
// Raises: ValueError - If *pos* is found to be out of range.
extern int8_t __hlt_bytes_pos_deref(__hlt_bytes_pos pos, __hlt_exception* excpt);

// Increases a position by one. If the given position is already the last
// one, the increased position will return True if compated with
// ~~__hlt_bytes_pos_end() via ~~__hlt_bytes_pos_eq(). If one tries to
// further increase such a position, it will be left untouched.
// 
// pos: The position to increase.
extern __hlt_bytes_pos __hlt_bytes_pos_incr(__hlt_bytes_pos pos, __hlt_exception* excpt);

// Compares two positions whether they refer to the same offset within a Bytes object. 
// 
// Returns: True if the position refer to the same location. Returns false in
// particular when *pos1* and *pos2* do not refer to the same Bytes object.
extern int8_t __hlt_bytes_pos_eq(__hlt_bytes_pos pos1, __hlt_bytes_pos pos2, __hlt_exception* excpt);

// Calculates the number of bytes between two position. Both positions must
// refer to the same Bytes object.
// 
// pos1: The starting position
// pos2: The end position; the byte pointed to by *pos* is not counted anymore. *end* must be >= *start*.
// 
// Returns: The number of bytes between *pos1* and *pos2*.
//
// Raises: ValueError - If any of the positions is found to be out of range.
extern __hlt_bytes_size __hlt_bytes_pos_diff(__hlt_bytes_pos pos1, __hlt_bytes_pos pos2, __hlt_exception* excpt);

// Converts a Bytes object into a String object. The function has the
// standard signature for string conversions.
// 
// Note: The conversion can be expensive. 
const __hlt_string* __hlt_bytes_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);

#endif
