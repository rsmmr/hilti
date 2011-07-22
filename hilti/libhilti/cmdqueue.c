// $Id$
//
// The command queue implementation.

#include <sys/prctl.h>

#include "hilti.h"
#include "tqueue.h"

static hlt_thread_queue* cmd_queue = 0;
static pthread_t queue_manager;

#define DBG_STREAM_QUEUE "hilti-queue"

static void fatal_error(const char* msg)
{
    fprintf(stderr, "command queue: %s\n", msg);
    exit(1);
}

// Executes a single command.
static void execute_cmd(__hlt_cmd *cmd, hlt_exception** excpt, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM_QUEUE, "executing cmd %p of type %d", cmd, cmd->type);

    switch ( cmd->type ) {
      case __HLT_CMD_FILE:
        __hlt_file_cmd_internal(cmd, excpt, ctx);
        break;

      default:
        fatal_error("undefined command type");
    }
}

// The entry function for the manager thread.
static void* _manager(void *arg)
{
    __hlt_thread_mgr_init_native_thread(__hlt_global_thread_mgr, "cqueue", 0);

    DBG_LOG(DBG_STREAM_QUEUE, "processing started");

    hlt_execution_context* ctx = hlt_execution_context_new(HLT_VID_QUEUE, 0);

    // We terminate iff all writer threads have terminated already with all
    // remaining elements processed.
    while ( ! hlt_thread_queue_terminated(cmd_queue) ) {

        __hlt_cmd* cmd = hlt_thread_queue_read(cmd_queue, 0);

        if ( ! cmd )
            continue;

        hlt_exception* excpt = 0;
        execute_cmd(cmd, &excpt, ctx);
        if ( excpt )
            __hlt_exception_print_uncaught_abort(excpt, ctx);

#if 1
        uint64_t size = hlt_thread_queue_size(cmd_queue);
        if ( size % 100 == 0 ) {
            char name_buffer[128];
            snprintf(name_buffer, sizeof(name_buffer), "cqueue (%lu)", size);
            name_buffer[sizeof(name_buffer)-1] = '\0';
            prctl(PR_SET_NAME, name_buffer, 0, 0, 0);
        }
#endif
    }

    DBG_LOG(DBG_STREAM_QUEUE, "command queue manager finished at size %d", hlt_thread_queue_size(cmd_queue));

    return 0;
}

void __hlt_cmd_queue_start()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "starting command queue manager thread");

    cmd_queue = hlt_thread_queue_new(hlt_config_get()->num_workers + 1, 1000, 0); // Slot 0 is main thread.

    if ( ! cmd_queue )
        fatal_error("cannot create command queue data structure");

    if ( pthread_create(&queue_manager, 0, _manager, 0) != 0 )
        fatal_error("cannot create cmd-queue thread");
}

void __hlt_cmd_worker_terminating(int worker)
{
    if ( ! hlt_is_multi_threaded() )
        return;

    hlt_thread_queue_terminate_writer(cmd_queue, worker);
}

void __hlt_cmd_queue_stop()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "waiting for command queue manager to terminate");

    hlt_thread_queue_terminate_writer(cmd_queue, 0); // The main thread.

    if ( pthread_join(queue_manager, 0) != 0 )
        fatal_error("cannot join thread");

    hlt_thread_queue_delete(cmd_queue);

    DBG_LOG(DBG_STREAM_QUEUE, "command queue manager has terminated");
}

void __hlt_cmd_queue_kill()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_QUEUE, "killing queue manager thread");

    pthread_cancel(queue_manager);
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
        execute_cmd(cmd, excpt, ctx);
    }

    else {
        DBG_LOG(DBG_STREAM_QUEUE, "queuing cmd %p of type %d", cmd, cmd->type);
        hlt_thread_queue_write(cmd_queue, ctx->worker ? ctx->worker->id : 0, cmd);
    }
}


