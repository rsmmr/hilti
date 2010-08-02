// $Id$
// 
// The command queue implementation.
// 
// TODO: We should move to a lock-free queue sometime. 

#include "hilti.h"

// The head and tail of the command queue. 
static __hlt_cmd* queue_head = 0;
static __hlt_cmd* queue_tail = 0;

static pthread_mutex_t queue_lock;     // Protects acceses to the queue data structure.
static pthread_cond_t queue_non_empty; // The manager will wait for this when the queue is empty.
static pthread_t queue_manager;        // The manager thread. 

static int queue_canceled = 0; // Will be set to signal termination request.

#define DBG_STREAM_THREADS "hilti-queue"
#define DBG_STREAM_QUEUE "hilti-queue"

static void fatal_error(const char* msg)
{
    fprintf(stderr, "command queue: %s\n", msg);
    exit(1);
}

// Executes a single command. 
static void execute_cmd(__hlt_cmd *cmd, hlt_exception** excpt)
{
    DBG_LOG(DBG_STREAM_QUEUE, "executing cmd %p of type %d", cmd, cmd->type);
    
    switch ( cmd->type ) {
      case __HLT_CMD_FILE:
        __hlt_file_cmd_internal(cmd, excpt);
        break;
        
      default:
        fatal_error("undefined command type");
    }
}

// The entry function for the manager thread. 
static void* _manager(void *arg)
{
    DBG_LOG(DBG_STREAM_THREADS, "command queue manager started");
    
    // We don't want to be canceled externally because we always want to
    // execute all pending commands before terminating. 
    if ( pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0) != 0 )
        fatal_error("cannot set cancel state");

    // We terminate iff termination has been requested and the queue is
    // empty.
    while ( queue_head || ! queue_canceled ) {
        
        __hlt_cmd* cmd = 0;
        
        if ( pthread_mutex_lock(&queue_lock) != 0 )
            fatal_error("cannot lock mutex");
        
        while ( ! queue_head && ! queue_canceled ) {
            if ( pthread_cond_wait(&queue_non_empty, &queue_lock) != 0 ) {
                fatal_error("wait for condition failed");
            }
        }

        if ( queue_head ) {
            cmd = queue_head;
            queue_head = cmd->next;
            
            if ( ! queue_head )
                queue_tail = 0;
        }

        if ( pthread_mutex_unlock(&queue_lock) != 0 )
            fatal_error("cannot unlock mutex");
        
        if ( cmd ) {
            hlt_exception* excpt = 0;
            execute_cmd(cmd, &excpt);
            if ( excpt )
                __hlt_exception_print_uncaught_abort(excpt);
        }
    }
    
    DBG_LOG(DBG_STREAM_THREADS, "command queue manager finished");
    
    return 0;
}
    
void __hlt_cmd_queue_start()
{
    if ( ! hlt_is_multi_threaded() )
        return;
    
    if ( pthread_mutex_init(&queue_lock, 0) != 0 )
        fatal_error("cannot init mutex");

    if ( pthread_cond_init(&queue_non_empty, 0) != 0 )
        fatal_error("cannot init condition variable");
    
    if ( pthread_create(&queue_manager, 0, _manager, 0) != 0 )
        fatal_error("cannot create worker threads");
}

void __hlt_cmd_queue_stop()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    DBG_LOG(DBG_STREAM_THREADS, "signaling command queue manager to stop");
    
    if ( pthread_mutex_lock(&queue_lock) != 0 )
        fatal_error("cannot lock mutex");

    queue_canceled = 1;

    if ( pthread_cond_signal(&queue_non_empty) != 0 )
        fatal_error("cannot signal condition variable");
    
    if ( pthread_mutex_unlock(&queue_lock) != 0 )
        fatal_error("cannot unlock mutex");
    
    if ( pthread_join(queue_manager, 0) != 0 )
        fatal_error("cannot join thread");
        
    if ( pthread_mutex_destroy(&queue_lock) != 0 )
        fatal_error("cannot destroy mutex");

    if ( pthread_cond_destroy(&queue_non_empty) != 0 )
        fatal_error("cannot destroy condition variable");
         
    DBG_LOG(DBG_STREAM_THREADS, "command queue manager has terminated");
}

void __hlt_cmdqueue_init_cmd(__hlt_cmd *cmd, uint16_t type)
{
    cmd->type = type;
    cmd->ctx = 0;
    cmd->next = 0;
}
    
void __hlt_cmdqueue_push(__hlt_cmd *cmd, hlt_exception** excpt)
{
    if ( ! hlt_is_multi_threaded() ) {
        execute_cmd(cmd, excpt);
        return;
    }

    DBG_LOG(DBG_STREAM_QUEUE, "queuing cmd %p of type %d", cmd, cmd->type);
    
    if ( pthread_mutex_lock(&queue_lock) != 0 )
        fatal_error("cannot lock mutex");

    if ( queue_tail ) {
        queue_tail->next = cmd;
        queue_tail = cmd;
    }
    else {
        if ( pthread_cond_signal(&queue_non_empty) != 0 )
            fatal_error("cannot signal condition variable");
            
        queue_head = queue_tail = cmd;
    }
    
    if ( pthread_mutex_unlock(&queue_lock) != 0 )
        fatal_error("cannot unlock mutex");
    
    return;
}

    
