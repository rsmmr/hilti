// $Id$
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
/// are also position objects, ~~hlt_bytes_pos. Each position is associated
/// with a particular bytes objects and can be used to locate a specific
/// byte. Creating a Position from an offset into the bytes can be
/// potentially expensive if the target position is far into the chunk list;
/// however once created, positions can be dererenced, incremented, and
/// decremented efficiently. 

#ifndef HILTI_bytes_H
#define HILTI_bytes_H

#include "exceptions.h"

typedef int32_t hlt_bytes_size;     ///< Size of a ~~hlt_bytes instance, and also used for offsets.
typedef struct hlt_bytes hlt_bytes; ///< Type for representing a HILTI ~~bytes object.
typedef struct hlt_bytes_chunk hlt_bytes_chunk;

/// A position with a ~~hlt_bytes instance.
typedef struct hlt_bytes_pos {
    hlt_bytes_chunk *chunk;  // Current chunk.
    const int8_t* cur;       // Current position in chunk.
    
    // Note: Initially, there was a 3rd pointer, end, pointing to chunk->end
    // to allow for quicker checks whether the end of the block has been
    // removed. However, LLVM has trouble passing struct with three pointers
    // by value are return value, on Darwin at least. However, even if it
    // could, it seems that the Darwin ABI specifies only two registers for
    // return values and so a larger struct would require memory allocation;
    // that's probably not worth the gain we get by having chunk->end
    // available directly.
} hlt_bytes_pos;

/// Instantiates a new bytes object. The bytes object is initially empty. 
/// 
/// Returns: The new bytes object. 
extern hlt_bytes* hlt_bytes_new(hlt_exception* excpt);

/// Instantiates a new bytes object from raw data. After instantiation, the
/// memory containing the raw data must not be modified or freed as the bytes
/// object might share it. 
/// 
/// data: Pointer to the raw bytes.
/// len: Number of raw byes starting at *data*.
/// excpt: &
/// 
/// Returns: The new bytes object. 
extern hlt_bytes* hlt_bytes_new_from_data(const int8_t* data, int32_t len, hlt_exception* excpt);
 
/// Returns the number of individual bytes stored in a bytes object.
/// 
/// b: The bytes object. 
/// 
/// excpt: &
/// 
/// Returns: The number of bytes stored in *b*.
/// 
/// Note: Calculating the length can potentially be expensive; it's not O(1).
extern hlt_bytes_size hlt_bytes_len(const hlt_bytes* b, hlt_exception* excpt);

/// Tests whether a bytes object is empty. 
/// 
/// b: The bytes object. 
///
/// excpt: &
/// 
/// Note: This test is more efficient than comparing the result of
/// ~~hlt_bytes_len() to zero. 
/// 
/// Returns: True if the object is empty.
extern int8_t hlt_bytes_empty(const hlt_bytes* b, hlt_exception* excpt);

/// Appends the contents of a bytes object to another. The two objects must not be the same.
/// 
/// b: The bytes object to append to.
/// other: The bytes object to append from.
/// excpt: &
/// 
/// Raises: ValueError - If the two objects are the same.
extern void  hlt_bytes_append(hlt_bytes* b, const hlt_bytes* other, hlt_exception* excpt);

/// Appends a sequence of raw bytes in memory to a bytes object. After
/// appending, the memory by the raw bytes must not be modified or freed as
/// the bytes object might share it. 
/// 
/// b: The bytes object to append to.
/// raw: A pointer to the beginning of the byte sequence to append. 
/// len: The number of bytes to append starting from *raw*.
/// excpt: &
extern void hlt_bytes_append_raw(hlt_bytes* b, const int8_t* raw, hlt_bytes_size len, hlt_exception* excpt);

/// Returns a subsequence of a bytes object. 
/// 
/// start: The start of the subsequence.
/// end: The end of the subsequence; *end* itself is not included anymore.
/// excpt: &
/// 
/// Returns: A new bytes object representing the subsequence.
/// 
/// Raises: ValueError - If any of the positions is found to be out of range.
extern hlt_bytes* hlt_bytes_sub(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt);

/// Returns a subsequence of a bytes object as a raw C array. 
/// 
/// start: The start of the subsequence. 
/// end: The end of the subsequence; *end* itself is not included anymore.
/// excpt: &
/// 
/// Returns: A pointer to continuous memory containing the subsequence. The
/// memory must not be altered nor freed. Only ``hlt_bytes_pos_diff(start,
/// end)`` number of bytes are valid. In particular, if *start* equals *end*
/// no byte must be read from the returned pointer.
/// 
/// Raises: ValueError - If any position is found to be out of range.
/// 
/// Note: This function can either be very efficient (if the substring is already located 
/// in continous memory), or expensive (if it's not, as then it needs to be copied). 
extern const int8_t* hlt_bytes_sub_raw(hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt);

/// Converts a bytes object into a raw C array. 
/// 
/// b: The object to convert. 
/// excpt: &
/// 
/// Returns: A pointer to continuous memory containing the bytes. The memory
/// must not be altered nor freed. Only ``hlt_bytes_len(b)`` number of bytes
/// are valid. 
/// 
/// Note: This function can either be very efficient (if the data is already
/// located in continous memory), or expensive (if it's not, as then it needs
/// to be copied). 
extern const int8_t* hlt_bytes_to_raw(const hlt_bytes* b, hlt_exception* excpt);

/// Returns one byte from a bytes object.
/// 
/// pos: The position from where to extract the byte. After reading the
/// byte, the iterator is incremented to the next position, or
/// hlt_bytes_end() if the end has been reached. If *pos* is already the
/// *end* position (or the end of the bytes object), it is left unchanged and
/// a ~~ValueError exception is raised.
/// 
/// end: End position. 
/// 
/// excpt: &
/// 
/// Raises: ValueError - If *pos* is already the end of bytes object. 
/// 
/// Note: This function is optimized for the case where you need a few
/// subsequent bytes from the string as well an iterator pointing to the
/// remainder of the bytes object. That's what is common with ``unpack``
/// implementations. 
/// 
/// Todo: Once we start compiling libhilti with llvm-gcc, calls to this function
/// should be optimized away. Check that. 
extern int8_t __hlt_bytes_extract_one(hlt_bytes_pos* pos, const hlt_bytes_pos end, hlt_exception* excpt);

/// Creates a new position object representing a specific offset.
/// 
/// b: The bytes object to create the position for.
/// excpt: &
/// 
/// offset: The offset within the object the position is to represent. The
/// offset is zero-based and must not extend beyond the length of the
/// stringth. If offset is negative, the length of the bytes object will be
/// added to it; in other words, negative offsets count from the end. 
/// 
/// Note: Determing the length can be expensive; it's not O(1).
/// 
/// Raises: ValueError - If *offset* is found to be out of range.
extern hlt_bytes_pos hlt_bytes_offset(const hlt_bytes* b, hlt_bytes_size offset, hlt_exception* excpt);
    
/// Returns a position representing the first element of a bytes object.    
/// 
/// b: The bytes object to return the beginning of.
/// excpt: &
/// 
/// Returns: The position.
extern hlt_bytes_pos hlt_bytes_begin(const hlt_bytes* b, hlt_exception* excpt);

/// Returns a position representing the end of any bytes object.
/// 
/// Returns: The position.
/// 
/// Note: This function does not receive a ~~bytes object because internally,
/// the end position is represented in a way independent of the actual
/// object.
extern hlt_bytes_pos hlt_bytes_end(hlt_exception* excpt);

/// Extracts the element at a position.  
/// 
/// pos: The position to dereference.
/// excpt: &
/// 
/// Returns: The element. 
/// 
/// Raises: ValueError - If *pos* is found to be out of range.
extern int8_t hlt_bytes_pos_deref(hlt_bytes_pos pos, hlt_exception* excpt);

/// Increases a position by one. If the given position is already the last
/// one, the increased position will return True if compated with
/// ~~hlt_bytes_pos_end() via ~~hlt_bytes_pos_eq(). If one tries to
/// further increase such a position, it will be left untouched.
/// 
/// pos: The position to increase.
/// excpt: &
extern hlt_bytes_pos hlt_bytes_pos_incr(hlt_bytes_pos pos, hlt_exception* excpt);

/// Increases a position by a given number of positions. If this exceeds the
/// number of available bytes, the return position will return True if
/// compared with ~~hlt_bytes_pos_end() via ~~hlt_bytes_pos_eq().
/// 
/// pos: The position to increase.
/// n: The number of bytes to skip.
/// excpt: &
extern hlt_bytes_pos hlt_bytes_pos_incr_by(hlt_bytes_pos pos, int32_t n, hlt_exception* excpt);

/// Compares two positions whether they refer to the same offset within a bytes object. 
///
/// pos1: The first position to be compared.
/// pos2: The second position to be compares.
/// excpt: &
/// 
/// Returns: True if the position refer to the same location. Returns false in
/// particular when *pos1* and *pos2* do not refer to the same bytes object.
extern int8_t hlt_bytes_pos_eq(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception* excpt);

/// Calculates the number of bytes between two position. Both positions must
/// refer to the same bytes object.
/// 
/// pos1: The starting position.
/// pos2: The end position; the byte pointed to by *pos* is not counted anymore. *end* must be >= *start*.
/// excpt: &
/// 
/// Returns: The number of bytes between *pos1* and *pos2*.
/// 
/// Raises: ValueError - If any of the positions is found to be out of range.
extern hlt_bytes_size hlt_bytes_pos_diff(hlt_bytes_pos pos1, hlt_bytes_pos pos2, hlt_exception* excpt);

/// Converts a bytes object into a string object.
///
/// Include: include-to-string-sig.txt
/// 
/// Note: The conversion can be expensive. 
hlt_string hlt_bytes_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt);

///< A block used for iteration by ~~hlt_bytes_iterate_raw.
struct hlt_bytes_block {
    const int8_t* start; ///< Pointer to first data byte of block
    const int8_t* end;   ///< Pointer to one beyond last data byte of block.
    
    hlt_bytes_chunk* next; // Pointer to next chunk.
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
/// excpt: &
/// 
/// Returns: Cookie for next call, or NULL if end has been reached. In the
/// latter case, block will still contain the final data (which may have a
/// length of zero); don't call the function again then.
void* hlt_bytes_iterate_raw(hlt_bytes_block* block, void* cookie, hlt_bytes_pos start, hlt_bytes_pos end, hlt_exception* excpt);

/// @}

#endif
