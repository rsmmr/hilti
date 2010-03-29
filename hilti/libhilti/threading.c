// $Id$
//
// Todo: 
// 
// - We should really use lock-free queues for the jobs. Looks like we hardly
// have any overlap between reads & writes for a busy queue. 
// 
// - We might want to consider a non-stack mapping of vhthreads to worker
// threads at some point. That's a major task however.

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "hilti.h"
#include "debug.h"

#define DBG_STREAM "hilti-threads"

struct hlt_worker_thread;

// A thread manager encapsulates the global state that all threads share.
struct hlt_thread_mgr {
    hlt_thread_mgr_state state;  // The manager's current state. 
    int num_workers;             // The number of worker threads.
    int num_excpts;              // The number of worker's that have raised exceptions.
    hlt_worker_thread** workers; // The worker threads.
    pthread_key_t id;            // A per-thread key storing a string identifying the string.
};

// A job queued for execution.
typedef struct hlt_job {
    hlt_continuation* func; // The bound function to run.
    hlt_vthread_id vid;     // The virtual thread the function is scheduled to.
    struct hlt_job* next;   // The next job in the queue.
} hlt_job;

// A struct that encapsulates data related to a single worker thread.
struct hlt_worker_thread {
    // Accesses to these must only be made from the worker thread itself.
    hlt_thread_mgr* mgr;          // The manager this thread is part of. 
    hlt_execution_context** ctxs; // Execution contexts indexed by virtual thread id.
    hlt_vthread_id max_vid;       // Largest vid allocated space for in ctxs.
    
    // This can be *read* from different threads without further locking.
    pthread_t handle;        // The pthread handle for this thread.

    // Accesses to these can be made from different threads but must be
    // coordinated via the lock (for both reads and writes). 
    hlt_job* jobs_head;      // The next job to be executed. 
    hlt_job* jobs_tail;      // The last job currently scheduled.
    
    pthread_mutex_t lock;   // The lock to protects accesses to these fields.
};

static void _fatal_error(const char* msg)
{
    fprintf(stderr, "libhilti threading: %s\n", msg);
    exit(1);
}

inline static void _init_lock(hlt_worker_thread* thread)
{
    if ( pthread_mutex_init(&thread->lock, 0) != 0 )
        _fatal_error("cannot initialize worker's lock");
}
inline static void _destroy_lock(hlt_worker_thread* thread)
{
    if ( pthread_mutex_destroy(&thread->lock) != 0 )
        _fatal_error("could not destroy lock");
}

// Note: When using _acquire/_release, it is important that there's no
// cancelation point between the two calls, as otherwise a lock main remain
// acquired after a thread has already terminated.
inline static void _acquire_lock(hlt_worker_thread* thread)
{
//    DBG_LOG(DBG_STREAM, "- attemptig to acquire lock for %p", thread);
    
    if ( pthread_mutex_lock(&thread->lock) != 0 )
        _fatal_error("cannot acquire lock");
    
//    DBG_LOG(DBG_STREAM, "- got lock for %p", thread);
}

inline static void _release_lock(hlt_worker_thread* thread)
{
//    DBG_LOG(DBG_STREAM, "- releasing lock for %p", thread);
    
    if ( pthread_mutex_unlock(&thread->lock) != 0 )
        _fatal_error("cannot release lock");
}

inline static void _atomic_increase(int* i)
{
    __sync_fetch_and_add(i, 1);
}

inline static int _atomic_fetch(int* i)
{
    // Let's assume this is atomic ...
    return *i;
}

// Cancels all worker threads. Safe to call from all threads.
void _cancel_threads(hlt_thread_mgr* mgr)
{
    for ( int i = 0; i < mgr->num_workers; i++ )
        // No error reporting since a thread may have received a cancel from
        // somewhere else already.
        pthread_cancel(mgr->workers[i]->handle);
}

// This function sleeps for a while and then kills all other workers. It will
// be run in its own thread, as that allows us to exit immediately once all
// workers have terminated. 
static void* _termination_watchdog(void* mgr_ptr)
{
    hlt_thread_mgr* mgr = (hlt_thread_mgr*) mgr_ptr;

    if ( pthread_setspecific(mgr->id, "watchdog") != 0 )
        _fatal_error("cannot set thread-local key");
    
    DBG_LOG(DBG_STREAM, "processing started");
    
    // Prepare the nanosleep data structure.
    double secs = hlt_config_get()->time_terminate;
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) (secs - sleep_time.tv_sec) * 1e9;
    
    // Sleep. We may be cancelled during this time if we're no longer needed.
    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;

    DBG_LOG(DBG_STREAM, "expired");
    
    DBG_LOG(DBG_STREAM, "canceling all threads");
    _cancel_threads(mgr);
    return 0;
}

// Terminates all worker threads. If graceful termination is requested, we
// give the workers a bit of time to shutdown themselves before we kill them.
// After the function returns, all worker threads will have terminated.
static void _terminate_threads(hlt_thread_mgr* mgr, int graceful)
{
    DBG_LOG(DBG_STREAM, "terminating all threads %s", (graceful ? "gracefully" : "immediately"));
    
    pthread_t watchdog;
    
    if ( graceful ) {
        // Start the watchdog thread to give them a chance to stop by themselves.
        if ( pthread_create(&watchdog, 0, _termination_watchdog, (void*) mgr) != 0 )
            _fatal_error("cannot create watchdog thread");
    }
    
    else {
        // Just kill them. 
        DBG_LOG(DBG_STREAM, "canceling all threads");
        _cancel_threads(mgr);
    }

    // Join all the workers. 
    for ( int i = 0; i < mgr->num_workers; i++ ) {
        DBG_LOG(DBG_STREAM, "joining worker %p", mgr->workers[i]);
        if ( pthread_join(mgr->workers[i]->handle, 0) != 0 )
            _fatal_error("cannot join worker thread");
    }
    
    if ( graceful ) {
        // Cancel the watchdog thread. No error reporting since the watchdog
        // thread may have terminated by itself by now.
        pthread_cancel(watchdog);
        
        DBG_LOG(DBG_STREAM, "joining watchdog");
        if ( pthread_join(watchdog, 0) != 0 )
            _fatal_error("cannot join watchdog thread");
    }
}

// Schedules a job to a worker thread.
static void _worker_schedule(hlt_worker_thread* current, hlt_worker_thread* target, hlt_vthread_id vid, hlt_continuation* func)
{
    hlt_job* job = hlt_gc_malloc_non_atomic(sizeof(hlt_job));
    job->func = func;
    job->vid = vid;
    job->next = 0;

    DBG_LOG(DBG_STREAM, "scheduling job %p for vid %d to worker %p", job, vid, target);
    
    _acquire_lock(target);
    
    if ( target->jobs_tail ) {
        target->jobs_tail->next = job;
        target->jobs_tail = job;
    }
    else 
        target->jobs_head = target->jobs_tail = job;
    
    _release_lock(target);
}

// Called when a virtual thread executes a ``yield`` statement. The first two
// arguments will always be 0. The contexts ``resume`` field will be set to a
// continuation to continue with when resuming.
static void _worker_yield(void* frame, void* eoss, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM, "vid %d is yielding", ctx->vid);
    _worker_schedule(ctx->worker, ctx->worker, ctx->vid, ctx->resume);
}

// Returns the execution context to use for a given virtual thread ID.
static hlt_execution_context* _worker_get_ctx(hlt_worker_thread* thread, hlt_vthread_id vid)
{
    hlt_vthread_id max = thread->max_vid;

    if ( max < vid ) {
        // Need to grow the context array.
        hlt_vthread_id new_max = max;
        while ( new_max < vid )
            new_max *= 2;
        
        thread->ctxs = hlt_gc_realloc_non_atomic(thread->ctxs, (new_max+1) * sizeof(hlt_execution_context*));
        if ( ! thread->ctxs )
            _fatal_error("cannot realloc context array");
        
        memset(thread->ctxs + max + 1, 0, (new_max - max) * sizeof(hlt_execution_context*));
        
        thread->max_vid = new_max;
    }

    hlt_execution_context* ctx = thread->ctxs[vid];
    
    if ( ! ctx ) {
        // Haven't seen this thread yet, need to create new context.
        ctx = hlt_execution_context_new(vid, _worker_yield);
        ctx->worker = thread;
        thread->ctxs[vid] = ctx;
    }

    return ctx;
}

static void _worker_run_job(hlt_worker_thread* thread, hlt_job* job)
{
    hlt_execution_context* ctx = _worker_get_ctx(thread, job->vid);
    DBG_LOG(DBG_STREAM, "executing job %p with context %p", job, ctx);
    
    hlt_exception* excpt = 0;
    
    hlt_call_continuation(job->func, ctx, &excpt);

    DBG_LOG(DBG_STREAM, "done with job %p", job);

    if ( ! excpt )
        // All fine.
        return;

    DBG_LOG(DBG_STREAM, "uncaught exception in worker thread");
    
    hlt_exception_print_uncaught_in_thread(excpt, job->vid);
    _atomic_increase(&thread->mgr->num_excpts);
    
    // We shutdown all threads. 
    DBG_LOG(DBG_STREAM, "canceling all threads");
    _cancel_threads(thread->mgr);
}

// Entry function for the worker threads.
static void* _worker(void* worker_thread_ptr)
{
    hlt_worker_thread* thread = (hlt_worker_thread*) worker_thread_ptr;
    hlt_job* job;

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worker %p", thread);
    if ( pthread_setspecific(thread->mgr->id, buffer) != 0 )
        _fatal_error("cannot set thread-local key");
    
    DBG_LOG(DBG_STREAM, "processing started", thread);
        
    // Prepare the nanosleep data structure.
    double secs = hlt_config_get()->time_idle;
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) (secs - sleep_time.tv_sec) * 1e9;

    while ( 1 ) {
        pthread_testcancel();
        
        _acquire_lock(thread);

        if ( thread->jobs_head ) {
            job = thread->jobs_head;
            
            thread->jobs_head = job->next;
            if ( ! thread->jobs_head )
                thread->jobs_tail = 0;
        
            _release_lock(thread);
                
            // Execute the job.
            _worker_run_job(thread, job);
        }
        
        else {
            // Idle. 
            if ( thread->mgr->state != HLT_THREAD_MGR_STOP ) {
                // Just sleep a bit.
                _release_lock(thread);
                nanosleep(&sleep_time, NULL);
                continue;
            }
            
            // Termination requested. If the job queue is empty at this
            // point, we terminate. Otherwise, we need to process the
            // remaining jobs in the queue.
            if ( ! thread->jobs_head ) {
                _release_lock(thread);
                DBG_LOG(DBG_STREAM, "exiting after stop request");
                return 0;
            }
            
            _release_lock(thread);
        }

    }
    
    __fatal_error("cannot be reached");
    return 0;
}

// Maps a vthread onto an actual worker thread. An FNV-1a hash is used to
// distribute the vthreads as evenly as possible between the worker threads.
// This algorithm should be fairly fast, but if profiling reveals
// hlt_thread_from_vthread to be a bottleneck, it can always be replaced with
// a simple mod function. The desire to perform this mapping quickly should
// be balanced with the desire to distribute the vthreads equitably, however,
// as an uneven distribution could result in serious performance issues for
// some applications.
static hlt_worker_thread* _vthread_to_worker(hlt_thread_mgr* mgr, hlt_vthread_id vid)
{
    // Some constants for the 32-bit FNV-1 hash algorithm.
    const uint32_t FNV_32_OFFSET_BASIS = 2166136261;
    const uint32_t FNV_32_PRIME = 16777619;
    const uint32_t FNV_32_MASK = 255;

    // Initialize.
    uint32_t hash = FNV_32_OFFSET_BASIS;

    // Perform a 32-bit FNV-1 hash.
    for ( unsigned i = 0; i < 4; i++ ) {
        hash = hash ^ ((vid >> (8 * i)) & FNV_32_MASK);
        hash = hash * FNV_32_PRIME;
    }

    // Map the hash onto the number of worker threads we actually have. Note
    // that using mod here introduces a slight amount of bias, but the
    // non-biased algorithm makes the time taken by this function
    // unpredictable. Further investigation may be warranted, but for now
    // I've used mod, which should still exhibit low bias and completes in a
    // fixed amount of time regardless of the input.
    return mgr->workers[hash % mgr->num_workers];
}

int8_t hlt_is_multi_threaded()
{
    return __hlt_global_thread_mgr != 0;
}

void hlt_threading_start()
{
    if ( hlt_config_get()->num_workers == 0 ) {
        DBG_LOG(DBG_STREAM, "no worker threads configured");
        // No threading configured. 
        return;
    }

    DBG_LOG(DBG_STREAM, "initializing threading system");
        
    __hlt_global_thread_mgr = hlt_thread_mgr_new();
    if ( ! __hlt_global_thread_mgr )
        _fatal_error("cannot create threading manager");
}

void hlt_threading_stop(hlt_exception** excpt)
{
    if ( ! __hlt_global_thread_mgr )
        return;

    DBG_LOG(DBG_STREAM, "stopping threading system");
    
    // Shutdown the worker threads. If we don't have any exceptions, we
    // give them a chance to shutdown gracefully, otherwise we just kill
    // the threads. 
    hlt_thread_mgr_set_state(__hlt_global_thread_mgr, *excpt ? HLT_THREAD_MGR_KILL : HLT_THREAD_MGR_STOP);

    // Now that all threads have terminated, we can check for any uncaught
    // exceptions and then destroy the thread manager.
    if ( ! *excpt )
        hlt_thread_mgr_check_exceptions(__hlt_global_thread_mgr, excpt);
    
    hlt_thread_mgr_destroy(__hlt_global_thread_mgr);

    DBG_LOG(DBG_STREAM, "threading system terminated");
    
    __hlt_global_thread_mgr = 0;
}

hlt_thread_mgr* hlt_thread_mgr_new()
{
    // Create the manager object.
    hlt_thread_mgr* mgr = hlt_gc_malloc_non_atomic(sizeof(hlt_thread_mgr));
    if ( ! mgr )
        _fatal_error("cannot allocate memory for thread manager");

    int num = hlt_config_get()->num_workers;
    if ( ! num )
        _fatal_error("no worker threads configured");
    
    mgr->state = HLT_THREAD_MGR_RUN;
    mgr->num_workers = num;
    mgr->num_excpts = 0;
    
    mgr->workers = hlt_gc_malloc_non_atomic(sizeof(hlt_worker_thread*) * num);
    if ( ! mgr->workers )
        _fatal_error("cannot allocate memory for worker threads");

    if ( pthread_key_create(&mgr->id, 0) != 0 )
        _fatal_error("cannot create thread-local key");

    if ( pthread_setspecific(mgr->id, "main thread") != 0 )
        _fatal_error("cannot set thread-local key");
    
    // Initialize the worker threads.
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, hlt_config_get()->stack_size);

    for ( int i = 0 ; i < num ; i++ ) {
        hlt_worker_thread* thread = hlt_gc_malloc_non_atomic(sizeof(hlt_worker_thread));
        thread->mgr = mgr;
        thread->jobs_head = 0;
        thread->jobs_tail = 0;
        thread->ctxs = hlt_gc_calloc_non_atomic(3, sizeof(hlt_execution_context*));
        thread->max_vid = 2;
        
        mgr->workers[i] = thread;
        
        _init_lock(thread);
        
        if ( pthread_create(&thread->handle, &attr, _worker, (void*) thread) != 0 )
            _fatal_error("cannot create worker threads");
        
        DBG_LOG(DBG_STREAM, "created worker thread %d at %p", i, thread);
    }
    
    pthread_attr_destroy(&attr);

    return mgr;
}

void hlt_thread_mgr_destroy(hlt_thread_mgr* mgr)
{
    DBG_LOG(DBG_STREAM, "destroying");
    
    if ( mgr->state != HLT_THREAD_MGR_DEAD )
        _fatal_error("thread manager being destroyed still has running threads");

    if ( pthread_key_delete(mgr->id) != 0 )
        _fatal_error("cannot delete thread-local key");
    
    for ( int i = 0; i < mgr->num_workers; i++ ) {
        hlt_worker_thread* thread = mgr->workers[i];
        
        _destroy_lock(thread);
        
        thread->jobs_head = thread->jobs_tail = 0;
    }
}

void hlt_thread_mgr_set_state(hlt_thread_mgr* mgr, const hlt_thread_mgr_state new_state)
{
    // If we're not making a change, do nothing.
    if (mgr->state == new_state)
        return;
    
    if ( mgr->state != HLT_THREAD_MGR_RUN )
        _fatal_error("hlt_set_thread_mgr_state called in state other than RUN");

    if ( new_state != HLT_THREAD_MGR_STOP && new_state != HLT_THREAD_MGR_KILL )
        _fatal_error("hlt_set_thread_mgr_state can only transition to STOP or KILL");

    DBG_LOG(DBG_STREAM, "transitioning from mgr state %d to %d", mgr->state, new_state);
    
    mgr->state = new_state;
        
    _terminate_threads(mgr, new_state == HLT_THREAD_MGR_STOP);
        
    // Update the run state to reflect the fact that the threads are dead.
    mgr->state = HLT_THREAD_MGR_DEAD;
    
    DBG_LOG(DBG_STREAM, "all threads terminated");
}

hlt_thread_mgr_state hlt_thread_mgr_get_state(const hlt_thread_mgr* mgr)
{
    return mgr->state;
}

uint32_t hlt_thread_mgr_num_threads(hlt_thread_mgr* mgr)
{
    return mgr->num_workers;
}

void __hlt_thread_mgr_schedule(hlt_thread_mgr* mgr, hlt_vthread_id vid, hlt_continuation* func, hlt_execution_context* ctx, hlt_exception** excpt)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return;
    }
    
    hlt_worker_thread* thread = _vthread_to_worker(mgr, vid);
    
    // Only schedule a job if we're in the RUN state.
    if ( mgr->state == HLT_THREAD_MGR_RUN )
        _worker_schedule(ctx->worker, thread, vid, func);
}

void hlt_thread_mgr_check_exceptions(hlt_thread_mgr* mgr, hlt_exception** excpt)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return;
    }
    
    if ( _atomic_fetch(&mgr->num_excpts) == 0 )
        return;

    DBG_LOG(DBG_STREAM, "raising UncaughtThreadException");
    hlt_set_exception(excpt, &hlt_exception_uncaught_thread_exception, 0);
}

const char* hlt_thread_mgr_current_native_thread(hlt_thread_mgr* mgr)
{
    if ( ! mgr )
        return "no threading";
    
    const char* id = pthread_getspecific(mgr->id);
    return id ? id : "no threading";
}
