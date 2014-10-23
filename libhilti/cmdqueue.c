//
// The command queue implementation.
//

#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>

#include "cmdqueue.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "globals.h"
#include "system.h"
#include "threading.h"
#include "tqueue.h"

#define DBG_STREAM_QUEUE "hilti-queue"

static void fatal_error(const char* msg)
{
    fprintf(stderr, "command queue: %s\n", msg);
    exit(1);
}

// Executes a single command.
static void execute_cmd(__hlt_cmd *cmd, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM_QUEUE, "executing cmd %p of type %d", cmd, cmd->type);

    switch ( cmd->type ) {
      case __HLT_CMD_FILE:
        __hlt_file_cmd_internal(cmd, ctx);
        break;

      default:
        fatal_error("undefined command type");
    }

    hlt_free(cmd);
}

// The entry function for the manager thread.
static void* _manager(void *arg)
{
    __hlt_thread_mgr_init_native_thread(hlt_global_thread_mgr(), "cqueue", 0);

    DBG_LOG(DBG_STREAM_QUEUE, "processing started");

    __hlt_files_init();

    hlt_execution_context* ctx = __hlt_globals()->cmd_context;

    // We terminate iff all writer threads have terminated already with all
    // remaining elements processed.
    while ( ! hlt_thread_queue_terminated(__hlt_globals()->cmd_queue) ) {

        __hlt_cmd* cmd = hlt_thread_queue_read(__hlt_globals()->cmd_queue, 0);

        if ( ! cmd )
            continue;

        execute_cmd(cmd, ctx);
        hlt_memory_safepoint(ctx);

        uint64_t size = hlt_thread_queue_size(__hlt_globals()->cmd_queue);
        if ( size % 100 == 0 ) {
            char name_buffer[128];
            snprintf(name_buffer, sizeof(name_buffer), "cqueue (%" PRIu64 ")", size);
            name_buffer[sizeof(name_buffer)-1] = '\0';
            hlt_set_thread_name(name_buffer);
        }
    }

    __hlt_files_done();

    DBG_LOG(DBG_STREAM_QUEUE, "command queue manager finished at size %d", hlt_thread_queue_size(__hlt_globals()->cmd_queue));

    return 0;
}

void __hlt_cmd_queue_init()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "starting command queue manager thread");

    __hlt_globals()->cmd_queue = hlt_thread_queue_new(hlt_config_get()->num_workers + 1, 1000, 0); // Slot 0 is main thread.
    __hlt_globals()->cmd_context = __hlt_execution_context_new_ref(HLT_VID_CMDQUEUE, 0);

    if ( ! __hlt_globals()->cmd_queue )
        fatal_error("cannot create command queue data structure");

    if ( pthread_create(&__hlt_globals()->queue_manager, 0, _manager, 0) != 0 )
        fatal_error("cannot create cmd-queue thread");
}

void __hlt_cmd_worker_terminating(int worker)
{
    if ( ! hlt_is_multi_threaded() )
        return;

    hlt_thread_queue_terminate_writer(__hlt_globals()->cmd_queue, worker);
}

void __hlt_cmd_queue_done()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "waiting for command queue manager to terminate");

    hlt_thread_queue_terminate_writer(__hlt_globals()->cmd_queue, 0); // The main thread.

    if ( pthread_join(__hlt_globals()->queue_manager, 0) != 0 )
        fatal_error("cannot join thread");

    hlt_thread_queue_delete(__hlt_globals()->cmd_queue);
    hlt_execution_context_delete(__hlt_globals()->cmd_context);

    DBG_LOG(DBG_STREAM_QUEUE, "command queue manager has terminated");
}

void __hlt_cmd_queue_kill()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "killing queue manager thread");

    pthread_cancel(__hlt_globals()->queue_manager);
}

void __hlt_cmdqueue_init_cmd(__hlt_cmd *cmd, uint16_t type)
{
    cmd->type = type;
    cmd->ctx = 0;
    cmd->next = 0;
}

void __hlt_cmdqueue_push(__hlt_cmd *cmd, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! hlt_is_multi_threaded() ) {
        DBG_LOG(DBG_STREAM_QUEUE, "directly executing cmd %p of type %d", cmd, cmd->type);
        execute_cmd(cmd, ctx);
    }

    else {
        DBG_LOG(DBG_STREAM_QUEUE, "queuing cmd %p of type %d", cmd, cmd->type);
        hlt_thread_queue_write(__hlt_globals()->cmd_queue, ctx->worker ? ctx->worker->id : 0, cmd);
    }
}


