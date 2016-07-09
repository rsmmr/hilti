
// Run-time support for the profiling instructions.

#ifndef HLT_PROFILER_H
#define HLT_PROFILER_H

#include <stdint.h>
#include <time.h>

#include "enum.h"
#include "types.h"

extern void hlt_profiler_start(hlt_string tag, hlt_enum style, uint64_t param, hlt_timer_mgr* tmgr,
                               hlt_exception** excpt, hlt_execution_context* ctx);
extern void hlt_profiler_update(hlt_string tag, uint64_t user_delta, hlt_exception** excpt,
                                hlt_execution_context* ctx);
extern void hlt_profiler_stop(hlt_string tag, hlt_exception** excpt, hlt_execution_context* ctx);

extern void __hlt_profiler_state_delete(__hlt_profiler_state* state);

extern void __hlt_profiler_init();
extern void __hlt_profiler_done();

// Cookie for timer-based snapshots.
typedef hlt_string __hlt_profiler_timer_cookie;

// Called when a snapshot timer fires.
extern void hlt_profiler_timer_expire(hlt_string tag, hlt_exception** excpt,
                                      hlt_execution_context* ctx);

////// Support for reading the profiling output.

static const uint64_t HLT_PROFILER_VERSION = 1; // File format version.

static const uint8_t HLT_PROFILER_START = 1;    // profiler.start
static const uint8_t HLT_PROFILER_UPDATE = 2;   // profiler.update
static const uint8_t HLT_PROFILER_SNAPSHOT = 3; // Snapshot condition triggered.
static const uint8_t HLT_PROFILER_STOP = 4;     // profiler.stop

// One profile record.
typedef struct {
    uint64_t ctime;   // Current time according to timer mgr (nsecs since epoch).
    uint64_t cwall;   // Current wall time (nsecs since epoch).
    uint64_t time;    // Time delta.
    uint64_t wall;    // Wall time delta.
    uint64_t updates; // Number of update calls.
    uint64_t cycles;  // CPU cycles/
    uint64_t misses;  // Cache misses so far.
    uint64_t alloced; // Heap change.
    uint64_t heap;    // Memory allocated (RSS).
    uint64_t user;    // Value of user's counter.
    uint8_t type;     // HLT_PROFILER_*.
} __attribute__((__packed__)) hlt_profiler_record;

static const int HLT_PROFILER_MAX_TAG_LENGTH = 256;

extern int hlt_profiler_file_open(const char* fname, time_t* t);
extern int hlt_profiler_file_read(int fd, char* tag,
                                  hlt_profiler_record* record); // This returns ASCII.
extern void hlt_profiler_file_close(int fd);

#endif
