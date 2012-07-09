
#ifndef HLT_TYPES_H
#define HLT_TYPES_H

#include <stdint.h>

// Forward declare a few of our core types here to avoid dependencies.
typedef struct __hlt_string* hlt_string;
typedef struct __hlt_exception hlt_exception;
typedef struct __hlt_execution_context hlt_execution_context;
typedef struct __hlt_type_info hlt_type_info;
typedef struct __hlt_thread_mgr hlt_thread_mgr;
typedef struct __hlt_callable hlt_callable;
typedef struct __hlt_timer_mgr hlt_timer_mgr;

typedef struct __hlt_profiler_state __hlt_profiler_state;

/// Type for hash values.
typedef uint64_t hlt_hash;

/// Type for the unique IDs of virtual threads.
typedef int64_t hlt_vthread_id;

// FIXME: Will move to threading.h once we have that.
#define HLT_VID_MAIN -1

#endif
