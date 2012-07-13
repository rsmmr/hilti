///
/// \addtogroup bytes
/// @{
///
/// Functions for manipulating file objects. File are explicitly thread-safe.
/// Writing to the same file from multiple threads is well-defined.
///
/// Internally, we push all writes into the internal command queue, and the
/// queue manager performs the actual output.
///
/// Note: For now, we only do output. Long-term, we could look into input via
/// a specialized IOSource.
///

#ifndef HILTI_FILE_H
#define HILTI_FILE_H

#include "enum.h"
#include "bytes.h"
#include "string_.h"
#include "cmdqueue.h"

typedef struct __hlt_file hlt_file;

// The file mode must match the Hilti::FileMode* constants in hilti.hlt.

/// When opening a file with this mode, an already existing file with the
/// same name will be overwritten. This is the default if no
/// ``HLT_FILE_OPEN_*`` mode is specified.
#define HLT_FILE_OPEN_CREATE  1

/// When opening a file with this mode, an already existing file will be
/// extended, not overwritten.
#define HLT_FILE_OPEN_APPEND  2

/// When opening a file in text mode, each write operation will be terminated
/// with a newline character. Furthermore, all non-printable characters will
/// be suitably escaped. This is the default if no ``HLT_FILE_TYPE_*`` mode
/// is specified.
#define HLT_FILE_TYPE_TEXT    4

/// When opening a file in binary mode, write operation will output all bytes
/// unmodified.
#define HLT_FILE_TYPE_BINARY  8

/// Instantiates a new file object, which will initially be closed and not
/// associated with any actual file.
///
/// excpt: &
///
/// Returns: A pointer to the opened file.
hlt_file* hlt_file_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Opens a file for writing.
///
/// path: The path of the file. If not absolute, it's interpreted relative to
/// the current directory.
///
/// file: The file to open.
///
/// type: ``Hilti_FileType_Text`` or ``Hilti_FileType_Binary``
///
/// mode: ``Hilti_FileMode_Create`` or ``Hilti_FileMode_Append``
///
/// charset: The charset to encode strings in when writing them into the file.
///
/// Raises: IOError if there was a problem opening the file.
///
/// Note: The HILTI run-time opens each file only once at any time. If a
/// different file object has already opened the same path, the newly created
/// file will reference the same internal object and the file mode will be
/// ignored.
///
/// Todo: We aren't very good yet in figuring out whether two different paths
/// may reference the same physical file. Currently, we just do a simple
/// string comparision.
void hlt_file_open(hlt_file* file, hlt_string path, hlt_enum type, hlt_enum mode, hlt_string charset, hlt_exception** excpt, hlt_execution_context* ctx);

/// Closes a file. Further write operations will not be possible (unless
/// reopened).
///
/// file: The file to close.
///
/// excpt: &
///
/// Note: If other file objects are referencing the same physical file, they
/// won't be closed; further writes to them are fine.
void hlt_file_close(hlt_file* file, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes a string into a file. The string will be encoded according to the
/// charset given when the file was opened. If the file was opened in text
/// mode, the string will be automatically terminated by a newline.
///
/// It's guaranteed that a single call to this function is atomic; all
/// characters will be written out in one piece even if other threads are
/// performing writes concurrently.  Multiple independent write call may
/// however be interleaved with calls from other threads.
///
/// file: The file to write to.
///
/// str: The string to write.
///
/// excpt: &
void hlt_file_write_string(hlt_file* file, hlt_string str, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes a bytes object into a file. If the file was opened in text mode,
/// unprintable characters will be suitably escaped.
///
/// It's guaranteed that a single call to this function is atomic; all bytes
/// will be written out in one piece even if other threads are performing
/// writes concurrently.  Multiple independent write calls may however be
/// interleaved with calls from other threads.
///
/// file: The file to write to.
///
/// bytes: The bytes to write.
///
/// excpt: &
void hlt_file_write_bytes(hlt_file* file, struct __hlt_bytes* bytes, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal function called once at startup to initialize the file
// management.
void __hlt_files_init();

// Internal function called at termination to clean up the threading.
void __hlt_files_done();

typedef struct __hlt_cmd_write __hlt_cmd_write;

// Internal function to perform the actual write from the queue manager. This
// function is not thread-safe and must be called only from a single thread.
void __hlt_file_cmd_internal(__hlt_cmd* c);

/// @}

#endif
