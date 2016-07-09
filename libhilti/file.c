
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "autogen/hilti-hlt.h"
#include "file.h"
#include "globals.h"
#include "memory_.h"

// This struct describes one currently open file. We memory-manage this ourselves.
struct __hlt_file_info {
    hlt_string path; // The path of the file.
    int fd;          // The file descriptor.
    int writers;     // The number of file objects having the file open from the OS perspective.
    bool error;      // True if we run into an error.

    struct __hlt_file_info* next; // We keep them in a list.
    struct __hlt_file_info* prev;
};

// A HILTI file object. This is one reference-counted.
struct __hlt_file {
    __hlt_gchdr __gchdr;   // Header for memory management.
    __hlt_file_info* info; // The internal file object.
    hlt_string path;       // The path of the file; keep here as well for threads.
    hlt_enum charset;      // The charset for a text file.
    hlt_enum type;         // Hilti_FileType_* constant.
    int8_t open;           // 1 if open, 0 if closed.
};

// A single write command inserted into the command queue.
typedef struct __hlt_cmd_file {
    __hlt_cmd cmd;         // The common header for all commands.
    __hlt_file_info* info; // The file to write to.
    int type;   // 1 for writing data; 1 if it's a close command; and 2 for a change in parameters.
    char* data; // Bytes to write.
    int len;    // Number of bytes to write.

    hlt_enum param_type; // For type 0: The type.
    hlt_enum param_mode; // For type 0: The mode.
} __hlt_cmd_file;

void hlt_file_dtor(hlt_type_info* ti, hlt_file* f, hlt_execution_context* ctx)
{
    GC_DTOR(f->path, hlt_string, ctx);
}

static void fatal_error(const char* msg)
{
    fprintf(stderr, "file manager: %s\n", msg);
    exit(1);
}

static inline void acqire_lock(int* i)
{
    if ( ! hlt_is_multi_threaded() )
        return;

    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, i);

    if ( pthread_mutex_lock(&__hlt_globals()->files_lock) != 0 )
        fatal_error("cannot lock mutex");
}

static inline void release_lock(int i)
{
    if ( ! hlt_is_multi_threaded() )
        return;

    if ( pthread_mutex_unlock(&__hlt_globals()->files_lock) != 0 )
        fatal_error("cannot unlock mutex");

    hlt_pthread_setcancelstate(i, NULL);
}

void __hlt_files_init()
{
    if ( hlt_is_multi_threaded() && pthread_mutex_init(&__hlt_globals()->files_lock, 0) != 0 )
        fatal_error("cannot init mutex");
}

void __hlt_files_done()
{
    __hlt_file_info* info = __hlt_globals()->files;

    while ( info ) {
        close(info->fd);

        GC_DTOR(info->path, hlt_string, hlt_global_execution_context());

        __hlt_file_info* next = info->next;
        hlt_free(info);
        info = next;
    }

    if ( hlt_is_multi_threaded() && pthread_mutex_destroy(&__hlt_globals()->files_lock) != 0 )
        fatal_error("cannot destroy mutex");
}

static inline void _hlt_file_init(hlt_file* file, hlt_exception** excpt, hlt_execution_context* ctx)
{
    file->info = 0;
    file->open = 0;
}

hlt_file* hlt_file_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_file* file = GC_NEW(hlt_file, ctx);
    _hlt_file_init(file, excpt, ctx);
    return file;
}

void* hlt_file_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate,
                           hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GC_NEW_REF(hlt_file, ctx);
}

void hlt_file_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate,
                         hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_file* src = *(hlt_file**)srcp;
    hlt_file* dst = *(hlt_file**)dstp;

    // Not so great that we need to lock here, but we need to to increase the
    // writer count.
    //
    // TODO: can we make this an atomic variable instead?

    if ( src->info && src->open ) {
        int s = 0;
        acqire_lock(&s);
        ++src->info->writers;
        release_lock(s);
    }

    dst->info = src->info;
    dst->open = src->open;
    dst->charset = src->charset;
    dst->type = src->type;

    __hlt_clone(&dst->path, &hlt_type_info_hlt_string, &src->path, cstate, excpt, ctx);
}

void hlt_file_open(hlt_file* file, hlt_string path, hlt_enum type, hlt_enum mode, hlt_enum charset,
                   hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( file->open ) {
        hlt_string err = hlt_string_from_asciiz("file already open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err, ctx);
        return;
    }

    // Find, or create, the file's global state record.

    int s = 0;
    acqire_lock(&s);

    // First see whether we know this file already.
    //
    // TODO: We do a simple string comparision here. Should come up with
    // something smarter ...

    __hlt_file_info* info;
    for ( info = __hlt_globals()->files; info; info = info->next ) {
        if ( hlt_string_cmp(path, info->path, excpt, ctx) == 0 ) {
            ++info->writers;
            goto init_instance;
        }
    }

    info = hlt_malloc(sizeof(__hlt_file_info));
    info->fd = -1;
    info->path = hlt_string_copy(path, excpt, ctx);
    info->writers = 1;
    info->error = 0;
    info->prev = 0;
    info->next = __hlt_globals()->files;

    GC_CCTOR(info->path, hlt_string, ctx);

    if ( __hlt_globals()->files )
        __hlt_globals()->files->prev = info;

    __hlt_globals()->files = info;

init_instance:
    file->info = info;
    file->type = type;
    file->charset = charset;
    file->path = path;
    file->open = 1;

    GC_CCTOR(file->path, hlt_string, ctx);

    release_lock(s);

    // Send open command.
    __hlt_cmd_file* cmd = hlt_malloc(sizeof(__hlt_cmd_file));
    __hlt_cmdqueue_init_cmd((__hlt_cmd*)cmd, __HLT_CMD_FILE);
    cmd->info = file->info;
    cmd->type = 1; // Open.
    cmd->param_type = type;
    cmd->param_mode = mode;
    __hlt_cmdqueue_push((__hlt_cmd*)cmd, excpt, ctx);
}

void hlt_file_close(hlt_file* file, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! file->open ) {
        hlt_string err = hlt_string_from_asciiz("file not open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err, ctx);
        return;
    }

    __hlt_cmd_file* cmd = hlt_malloc(sizeof(__hlt_cmd_file));
    __hlt_cmdqueue_init_cmd((__hlt_cmd*)cmd, __HLT_CMD_FILE);
    cmd->info = file->info;
    cmd->type = 3; // Close.
    __hlt_cmdqueue_push((__hlt_cmd*)cmd, excpt, ctx);

    file->open = 0;
    GC_CLEAR(file->path, hlt_string, ctx);
}

hlt_string hlt_file_name(hlt_file* file, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return file->path;
}

void hlt_file_write_string(hlt_file* file, hlt_string str, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_string_encode(str, file->charset, excpt, ctx);
    if ( hlt_check_exception(excpt) )
        return;

    hlt_file_write_bytes(file, b, excpt, ctx);
}

static void _add_to_cmd_data(__hlt_cmd_file* cmd, const int8_t* data, int len)
{
    int old_len = cmd->len;
    cmd->len += len;
    cmd->data = hlt_realloc(cmd->data, cmd->len, old_len);
    memcpy(cmd->data + old_len, data, len);
}

void hlt_file_write_bytes(hlt_file* file, hlt_bytes* data, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! file->open ) {
        hlt_string err = hlt_string_from_asciiz("file not open", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_io_error, err, ctx);
        return;
    }

    __hlt_cmd_file* cmd = hlt_malloc(sizeof(__hlt_cmd_file));
    __hlt_cmdqueue_init_cmd((__hlt_cmd*)cmd, __HLT_CMD_FILE);
    cmd->info = file->info;
    cmd->type = 2; // Write.
    cmd->data = 0;
    cmd->len = 0;

    hlt_bytes_block block;
    hlt_iterator_bytes start = hlt_bytes_begin(data, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);
    void* cookie = 0;
    int8_t buf[5] = {'\\', 'x', 'X', 'X', '0'};

    while ( 1 ) {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);

        if ( block.start == block.end )
            break;

        int8_t text = hlt_enum_equal(file->type, Hilti_FileType_Text, excpt, ctx);

        if ( text ) {
            // Need to escape unprintable characters. FIXME: We don't honor
            // the charset here yet, just encode everything that is not
            // representable in our current locale.
            const int8_t* s = block.start;
            const int8_t* e = s;

            while ( s < block.end ) {
                while ( e < block.end && isprint(*e) )
                    ++e;

                if ( e != s )
                    _add_to_cmd_data(cmd, s, e - s);

                if ( e < block.end ) {
                    // Unprintable character.
                    int n = hlt_util_uitoa_n(*e, (char*)buf + 2, 3, 16, 1);
                    _add_to_cmd_data(cmd, buf, n + 2);
                    ++e;
                }

                s = e;
            }
        }

        else if ( hlt_enum_equal(file->type, Hilti_FileType_Binary, excpt, ctx) )
            // Just write data directly.
            _add_to_cmd_data(cmd, block.start, block.end - block.start);

        else
            fatal_error("unknown file type");

        if ( ! cookie )
            break;
    }

    if ( hlt_enum_equal(file->type, Hilti_FileType_Text, excpt, ctx) )
        _add_to_cmd_data(cmd, (int8_t*)"\n", 1);

    __hlt_cmdqueue_push((__hlt_cmd*)cmd, excpt, ctx);
}

void __hlt_file_cmd_internal(__hlt_cmd* c, hlt_execution_context* ctx)
{
    __hlt_cmd_file* cmd = (__hlt_cmd_file*)c;

    // This will be called from the command queue thread only.
    switch ( cmd->type ) {
    case 1: {
        if ( cmd->info->error )
            return;

        // Open command.
        int s = 0;
        acqire_lock(&s);

        if ( cmd->info->fd < 0 ) {
            // Not open yet.

            hlt_exception* excpt = 0;
            char* fn = hlt_string_to_native(cmd->info->path, &excpt, ctx);
            if ( excpt ) {
                release_lock(s);
                break;
            }

            int8_t append = hlt_enum_equal(cmd->param_mode, Hilti_FileMode_Append, &excpt, ctx);
            int oflags = O_CREAT | O_WRONLY | (append ? O_APPEND : O_TRUNC);
            int fd = open(fn, oflags, 0666);

            hlt_free(fn);

            if ( fd < 0 ) {
                cmd->info->error = 1;
                // TODO: We don't have a good way to report errors unfortunately.
                release_lock(s);
                return;
            }

            cmd->info->fd = fd;
        }

        release_lock(s);
        break;

    case 2: {
        // Write comment.

        if ( cmd->info->fd < 0 )
            return;

        if ( cmd->info->error )
            return;

        if ( cmd->len ) {
            if ( ! __hlt_safe_write(cmd->info->fd, cmd->data, cmd->len) )
                cmd->info->error = 1;

            hlt_free(cmd->data);
        }

        break;
    }

    case 3: {
        // Close command.

        if ( cmd->info->fd < 0 )
            return;

        int s = 0;
        acqire_lock(&s);

        assert(cmd->info->writers);

        if ( --cmd->info->writers == 0 ) {
            close(cmd->info->fd);

            // Delete from list.
            __hlt_file_info* cur;
            for ( cur = __hlt_globals()->files; cur; cur = cur->next ) {
                if ( cur != cmd->info )
                    continue;

                if ( cur->prev )
                    cur->prev->next = cur->next;
                else
                    __hlt_globals()->files = cur->next;

                if ( cur->next )
                    cur->next->prev = cur->prev;

                break;
            }

            if ( ! cur )
                fatal_error("file to close not found");

            GC_DTOR(cmd->info->path, hlt_string, hlt_global_execution_context());
            hlt_free(cmd->info);

            cmd->info = 0;
        }

        release_lock(s);
        break;
    }

    default:
        fatal_error("unknown data type in write");
    }
    }
}

hlt_string hlt_file_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                              __hlt_pointer_stack* seen, hlt_exception** excpt,
                              hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_FILE);
    hlt_file* file = *((hlt_file**)obj);

    if ( ! file )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("<file ", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);

    hlt_string s = hlt_string_concat(prefix, file->path, excpt, ctx);
    return hlt_string_concat(s, postfix, excpt, ctx);
}
