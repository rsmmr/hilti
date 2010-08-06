// $Id$

#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "hilti.h"

// This struct describes one currently open file.
typedef struct __hlt_file_info {
    hlt_string path;    // The path of the file. 
    int fd;             // The file descriptor.
    int writers;        // The number of file objects having the file open.
    
    struct __hlt_file_info* next; // We keep them in a list.
    struct __hlt_file_info* prev; 
} __hlt_file_info;

// A HILTI file object. 
struct hlt_file {
    __hlt_file_info* info; // The internal file object. 
    int type;              // Hilti_FileType_* constant.
    hlt_string charset;    // The charset for a text file.
    int8_t open;           // 1 if open, 0 if closed. 
};

// A single write command inserted into the command queue.
typedef struct __hlt_cmd_file {
    __hlt_cmd cmd;             // The common header for all commands. 
    hlt_file* file;            // The file to write to.
    int type;                  // 1 if it's a string; 0 if it's a bytes; and 2 if it's a close command.
    union {
        void* bytes;           // The bytes to write.
        void* string;          // The string to write.
    } data;
} __hlt_cmd_file;

// The list of currently open files. Read and write accesses to this list
// must be protected via the lock below.
static __hlt_file_info* files = 0;

// Lock to protect access to the list of open files;
pthread_mutex_t files_lock;   

static void fatal_error(const char* msg)
{
    fprintf(stderr, "file manager: %s\n", msg);
    exit(1);
}

static inline void acqire_lock()
{
    if ( ! hlt_is_multi_threaded() )
        return;
    
    if ( pthread_mutex_lock(&files_lock) != 0 )
        fatal_error("cannot lock mutex");
}
    
static inline void release_lock()
{
    if ( ! hlt_is_multi_threaded() )
        return;
    
    if ( pthread_mutex_unlock(&files_lock) != 0 )
        fatal_error("cannot unlock mutex");
}
    
void __hlt_files_start()
{
    if ( hlt_is_multi_threaded() && pthread_mutex_init(&files_lock, 0) != 0 )
        fatal_error("cannot init mutex");
}

void __hlt_files_stop()
{
    // if ( hlt_is_multi_threaded() && pthread_mutex_destroy(&files_lock) != 0 )
    //    fatal_error("cannot destroy mutex");
    
    // TODO: We don't destroy the mutex for now because the cmd-queue might
    // still need it when writing stuff out. The question is when can we
    // safely destroy it?
}

hlt_file* hlt_file_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_file* file = hlt_gc_malloc_non_atomic(sizeof(hlt_file));
    if ( ! file ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }
    
    file->info = 0;
    file->open = 0;
    return file;
}

void hlt_file_open(hlt_file* file, hlt_string path, int8_t type, int8_t mode, hlt_string charset, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( file->open ) {
        hlt_string err = hlt_string_from_asciiz("file already open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err);
        return;
    }
    
    acqire_lock();    
    
    // First see whether we know this file already.
    // 
    // TODO: We do a simple string comparision here. Should come up with
    // something smarter ...

    __hlt_file_info* info;
    for ( info = files; info; info = info->next ) {
        if ( hlt_string_cmp(path, info->path, excpt, ctx) == 0 ) {
            // Already open.
            ++info->writers;
            goto init_instance;
        }
    }
    
    // Not open yet. 

    const char* fn = hlt_string_to_native(path, excpt, ctx);
    if ( *excpt )
        return;
    
    int oflags = O_CREAT | O_WRONLY | (mode == Hilti_FileMode_Append ? O_APPEND : O_TRUNC);
    int fd = open(fn, oflags, 0777);
    
    if ( fd < 0 ) {
        hlt_string err = hlt_string_from_asciiz(strerror(errno), excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err);
        goto error;
    }

    info = hlt_gc_malloc_non_atomic(sizeof(__hlt_file_info));
    if ( ! info ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        goto error;
    }
    
    info->fd = fd;
    info->path = path;
    info->writers = 1;
    info->prev = 0;
    info->next = files;
    
    if ( files )
        files->prev = info;
    
    files = info;

init_instance:    
    file->info = info;
    file->type = type;
    file->charset = charset;
    file->open = 1;
    
    goto done;
    
error:    
    if ( file && file->info )
        --info->writers;

    file->info = 0;
    file->open = 0;
    
done:    
    release_lock();    
}

void hlt_file_close(hlt_file* file, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! file->open ) {
        hlt_string err = hlt_string_from_asciiz("file not open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err);
        return;
    }

    __hlt_cmd_file* cmd = hlt_gc_malloc_non_atomic(sizeof(__hlt_cmd_file));
    if ( ! cmd ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
    
    __hlt_cmdqueue_init_cmd((__hlt_cmd*) cmd, __HLT_CMD_FILE);
    cmd->file = file;
    cmd->type = 2; // Close.
    __hlt_cmdqueue_push((__hlt_cmd*) cmd, excpt, ctx);
    
    file->open = 0;
}

void hlt_file_write_string(hlt_file* file, hlt_string str, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! file->open ) {
        hlt_string err = hlt_string_from_asciiz("file not open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err);
        return;
    }
    
    __hlt_cmd_file* cmd = hlt_gc_malloc_non_atomic(sizeof(__hlt_cmd_file));
    if ( ! cmd ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
    
    __hlt_cmdqueue_init_cmd((__hlt_cmd*) cmd, __HLT_CMD_FILE);
    cmd->file = file;
    cmd->type = 1; // String.
    // Don't need to copy the string because it's not mutable.
    cmd->data.string = str;
    __hlt_cmdqueue_push((__hlt_cmd*) cmd, excpt, ctx);
}

void hlt_file_write_bytes(hlt_file* file, hlt_bytes* bytes, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! file->open ) {
        hlt_string err = hlt_string_from_asciiz("file not open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err);
        return;
    }
    
    // Need to copy the bytes because it's mutable.
    hlt_bytes* copy = hlt_bytes_copy(bytes, excpt, ctx);
    if ( *excpt )
        return;
    
    __hlt_cmd_file* cmd = hlt_gc_malloc_non_atomic(sizeof(__hlt_cmd_file));
    if ( ! cmd ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }
    
    __hlt_cmdqueue_init_cmd((__hlt_cmd*) cmd, __HLT_CMD_FILE);
    cmd->file = file;
    cmd->type = 0; // Bytes.
    cmd->data.bytes = copy;
    __hlt_cmdqueue_push((__hlt_cmd*) cmd, excpt, ctx);
}

static void _write_bytes(hlt_file* file, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_block block;
    hlt_bytes_pos start = hlt_bytes_begin(data, excpt, ctx);
    hlt_bytes_pos end = hlt_bytes_end(data, excpt, ctx);
    void* cookie = 0;
    
    assert(file->info);
    
    while ( true ) {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);
        
        if ( block.start == block.end )
            break;
        
        if ( file->type == Hilti_FileType_Text ) {
            // Escape unprintable characters. FIXME: We don't honor the
            // charset here yet, just encode everything that is not
            // representable in our current locale.
            for ( const int8_t* c = block.start; c < block.end; c++ ) {
                if ( isprint(*c) )
                    write(file->info->fd, c, 1);
                else {
                    char buf[5];
                    int n = snprintf(buf, sizeof(buf) - 1, "\\x%x", (int)*c);
                    write(file->info->fd, buf, n);
                }
            }
        }
        
        else if ( file->type == Hilti_FileType_Binary )
            // Just write data directly. 
            write(file->info->fd, block.start, block.end - block.start);
            
        else
            fatal_error("unknown file type");
            
        if ( ! cookie )
            break;
    }
    
    if ( file->type == Hilti_FileType_Text )
        write(file->info->fd, "\n", 1);
        
}

void __hlt_file_cmd_internal(__hlt_cmd* c, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_cmd_file* cmd = (__hlt_cmd_file*) c;
    
    // This will be called from the command queue thread only.
    switch ( cmd->type ) {
        
      case 0: {
          // Bytes.
          _write_bytes(cmd->file, cmd->data.bytes, excpt, ctx);
          break;
      }
        
      case 1: {
          // String.
          hlt_bytes* b = hlt_string_encode(cmd->data.string, cmd->file->charset, excpt, ctx);
          if ( *excpt )
              return;
          
          _write_bytes(cmd->file, b, excpt, ctx);
          break;
      }
        
      case 2: {
          // Close command. 
          acqire_lock();    

          assert(cmd->file->info->writers);
          
          if ( --cmd->file->info->writers == 0 ) {
              close(cmd->file->info->fd);
              // Clear out, shouldn't be used anymore now. 
              cmd->file->info->fd = -1;
          
              // Delete from list. 
              __hlt_file_info* cur;
              for ( cur = files; cur; cur = cur->next ) {
                  if ( cur != cmd->file->info )
                      continue;
                  
                  if ( cur->prev )
                      cur->prev->next = cur->next;
                  else
                      files = cur->next;
                  
                  if ( cur->next )
                      cur->next->prev = cur->prev;
                  
                  break;
              }
              
              if ( ! cur )
                  fatal_error("file to close not found");
          }
          
          release_lock();    
          break;
      }
        
      default:
        fatal_error("unknown data type in write");
    }
}


