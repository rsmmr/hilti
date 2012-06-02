//
// The command queue is an internal mechanism to serialize operations that
// must not be executed concurrently. Writers send commands to the queue, and
// a dedidacated manager threads then takes them out and executes the
// operations they describe.
//
// Note that communication is one way, there's no feedback from the queue
// manager to the threads issueing the commands.
//
// This interface is also available when running in a non-threaded
// environment; we then execute the commands synchronously when they are
// pushed into the queue.
//
// The queue mechanism is not visible at the HILTI level, this API is
// strictly internal.

#ifndef HILTI_CMDQUEUE_H
#define HILTI_CMDQUEUE_H

#include <stdint.h>

#include "context.h"
#include "exceptions.h"
#include "threading.h"

// Called from hlt_init().
extern void __hlt_cmd_queue_init();

// Called from hlt_done().
extern void __hlt_cmd_queue_done();

// Called from _kill_all_threads() in threading.cc.
extern void __hlt_cmd_queue_kill();

// The command types currently defined.
#define __HLT_CMD_FILE   1  // File operations.

// The common header for all commands. Custom data may follow this header in
// per-command structs.
typedef struct __hlt_cmd {
    uint16_t type;               // One of __HLT_CMD_*
    hlt_execution_context* ctx;  // The context of the writer (not set currently).
    struct __hlt_cmd* next;      // We keep them in a queue.
} __hlt_cmd;

// Initializes a command. This initializes the common ~~__hlt_cmd header.
//
// cmd: The command to initialize.
// type: One of __HLT_CMD_*.
void __hlt_cmdqueue_init_cmd(__hlt_cmd *cmd, uint16_t type);

// Writes a command for execution into the queue.
//
// cmd: The command to write into the queue. The function takes ownership.
//
// Note that in a non-threaded configuration, this will directly execute the
// command and return only after it has finished.
extern void __hlt_cmdqueue_push(__hlt_cmd *cmd, hlt_exception** excpt, hlt_execution_context* ctx);

// Signals that a worker thread is about to terminate.
extern void __hlt_cmd_worker_terminating(int worker);


#endif
