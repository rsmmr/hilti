// $Id$

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include "hilti.h"
#include "debug.h"
#include "rtti.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/prctl.h>
#include <pthread.h>

// Batch size for the jobs queues.
#define QUEUE_BATCH_SIZE 5000

// Number of pending element across all job qeueus that correspond to the
// maximum load of 1.0.
# define QUEUE_MAX_LOAD   (QUEUE_BATCH_SIZE * 3 * mgr->num_workers)

#ifdef DEBUG
uint64_t job_counter = 0;
#endif

struct hlt_worker_thread;

// A thread manager encapsulates the global state that all threads share.
struct hlt_thread_mgr {
    hlt_thread_mgr_state state;  // The manager's current state.
    int num_workers;             // The number of worker threads.
    int num_excpts;              // The number of worker's that have raised exceptions.
    hlt_worker_thread** workers; // The worker threads.
    pthread_t cmdqueue;          // The pthread handle for the command queue.
    pthread_key_t id;            // A per-thread key storing a string identifying the string.
};

// A job queued for execution.
struct hlt_job {
    hlt_continuation* func; // The bound function to run.
    hlt_vthread_id vid;     // The virtual thread the function is scheduled to.
    void* tcontext;         // The jobs thread context to use when executing.

#ifdef DEBUG
    uint64_t id;            // For debugging, we assign numerical IDs for easier identification.
#endif

    struct hlt_job* prev;   // The prev job in the *blocked* queue if linked in there.
    struct hlt_job* next;   // The next job in the *blocked* queue if linked in there.
};

static void _fatal_error(const char* msg)
{
    fprintf(stderr, "libhilti threading: %s\n", msg);
    exit(1);
}

// Cancels all threads immediately. Safe to call from all threads.
void _kill_all_threads(hlt_thread_mgr* mgr)
{
    DBG_LOG(DBG_STREAM, "canceling all threads");

    for ( int i = 0; i < mgr->num_workers; i++ )
        // No error reporting since a thread may have received a cancel from
        // somewhere else already.
        pthread_cancel(mgr->workers[i]->handle);

    __hlt_cmd_queue_kill();
}

// This function sleeps for a while and then kills all other workers. It will
// be run in its own thread as that allows us to exit immediately once all
// workers have terminated.
static void* _termination_watchdog(void* mgr_ptr)
{
    hlt_thread_mgr* mgr = (hlt_thread_mgr*) mgr_ptr;

    __hlt_thread_mgr_init_native_thread(mgr, "watchdog", 0);

    DBG_LOG(DBG_STREAM, "processing started");

    double secs = hlt_config_get()->time_terminate;
    hlt_util_nanosleep((int)(secs * 1e9));

    DBG_LOG(DBG_STREAM, "expired");

    _kill_all_threads(mgr);
    return 0;
}

// Terminates all worker threads. The specifics depend on the given state:
//
// FINISH: Terminate once all workers are idle.
//
// STOP: Stop workers from scheduling new jobs, and give them a bit of time
// to finish with what they are doing at the moment. If they exceed that
// interval, kill them.
//
// KILL: Kill them right away.
static void _terminate_threads(hlt_thread_mgr* mgr, hlt_thread_mgr_state state)
{
    DBG_LOG(DBG_STREAM, "terminating all threads (%d)", state);

    pthread_t watchdog;
    int watchdog_started = 0;

    mgr->state = state;

    // Terminate the main thread's write ends of the job queues.
    for ( int i = 0; i < mgr->num_workers; ++i )
        hlt_thread_queue_terminate_writer(mgr->workers[i]->jobs, 0);

    switch ( state ) {
      case HLT_THREAD_MGR_FINISH:
        DBG_LOG(DBG_STREAM, "waiting for all threads to become idle");

        // This loop is not free of race conditions. However, I claim that
        // the cases where it's unpredicatable are ill-defined to begin with! :)
        while ( true ) {
            int idle = 0;

            for ( int i = 0; i < mgr->num_workers; ++i ) {
                if ( mgr->workers[i]->idle && hlt_thread_queue_size(mgr->workers[i]->jobs) == 0 )
                    ++idle;
            }

            if ( idle == mgr->num_workers )
                break;

            hlt_util_nanosleep(1000);
        }

        mgr->state = state = HLT_THREAD_MGR_STOP;
        // Fall-through.

      case HLT_THREAD_MGR_STOP:
        DBG_LOG(DBG_STREAM, "waiting for all threads to stop");

        // Start the watchdog thread to give them a chance to stop by themselves.
        if ( pthread_create(&watchdog, 0, _termination_watchdog, (void*) mgr) != 0 )
            _fatal_error("cannot create watchdog thread");

        watchdog_started = 1;
        break;

      case HLT_THREAD_MGR_KILL:
        _kill_all_threads(mgr);
        break;

      default:
        _fatal_error("internal error: unknown state in terminate threads");
    }

    // Join all the workers.
    for ( i = 0; i < mgr->num_workers; i++ ) {
        DBG_LOG(DBG_STREAM, "joining %s", mgr->workers[i]->name);
        if ( pthread_join(mgr->workers[i]->handle, 0) != 0 )
            _fatal_error("cannot join worker thread");
    }

    if ( watchdog_started ) {
        // Cancel the watchdog thread. No error reporting since the watchdog
        // thread may have terminated by itself by now.
        pthread_cancel(watchdog);

        DBG_LOG(DBG_STREAM, "joining watchdog");
        if ( pthread_join(watchdog, 0) != 0 )
            _fatal_error("cannot join watchdog thread");
    }
}

static void _add_to_blocked(hlt_worker_thread* thread, hlt_job* job)
{
    if ( thread->blocked_tail )
        thread->blocked_tail->next = job;
    else
        thread->blocked_head = job;

    job->prev = thread->blocked_tail;
    job->next = 0;

    thread->blocked_tail = job;

    assert(job->func->blockable);
    ++job->func->blockable->num_blocked;
    ++thread->num_blocked_jobs;

    DBG_LOG(DBG_STREAM, "adding job %lu to blocked queue for %s with blockable %p", job->id, thread->name, job->func->blockable);
}

static void _remove_from_blocked(hlt_worker_thread* thread, hlt_job* job)
{
    if ( job->prev )
        job->prev->next = job->next;
    else
        thread->blocked_head = job->next;

    if ( job->next )
        job->next->prev = job->prev;
    else
        thread->blocked_tail = job->prev;

    job->prev = 0;
    job->next = 0;

    --job->func->blockable->num_blocked;
    --thread->num_blocked_jobs;

    DBG_LOG(DBG_STREAM, "removed job %lu from blocked queue for %s", job->id, thread->name);
}

// Schedules a job to a worker thread.
static void _worker_schedule_job(hlt_worker_thread* target, hlt_job* job, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM, "scheduling job %lu for vid %d to %s", job->id, job->vid, target->name);

    if ( ! job->func->blockable )
        hlt_thread_queue_write(target->jobs, ctx->worker ? ctx->worker->id : 0, job);
    else {
        assert(ctx->worker);
        _add_to_blocked(target, job);
    }
}

static void _worker_schedule(hlt_worker_thread* target, hlt_vthread_id vid, hlt_continuation* func, void* tcontext, hlt_execution_context* ctx)
{
    if ( target->mgr->state != HLT_THREAD_MGR_RUN && target->mgr->state != HLT_THREAD_MGR_FINISH ) {
        DBG_LOG(DBG_STREAM, "omitting scheduling of job because mgr signaled termination");
        return;
    }

    hlt_job* job = hlt_gc_malloc_non_atomic(sizeof(hlt_job));
    job->func = func;
    job->vid = vid;
    job->tcontext = tcontext,
    job->next = 0;
    job->prev = 0;
#if DEBUG
    job->id = ++job_counter;
#endif

    _worker_schedule_job(target, job, ctx);
}

#ifdef DEBUG

static void _debug_print_queue_stats(const hlt_thread_queue_stats* stats)
{
    fprintf(stderr, " elems=%lu  batches=%lu  blocked=%lu  locked=%lu\n",
            stats->elems, stats->batches, stats->blocked, stats->locked);
}

static void _debug_print_job_summary(hlt_thread_mgr* mgr)
{
    for ( int i = 0; i < mgr->num_workers; i++ ) {
        hlt_worker_thread* thread = mgr->workers[i];
        uint64_t size = hlt_thread_queue_pending(thread->jobs);

        DBG_LOG(DBG_STREAM_STATS, "%s: %d jobs currently queued", thread->name, size);

        hlt_thread_queue* queue = thread->jobs;
        fprintf(stderr, "=== %s\n", thread->name);
        fprintf(stderr, "  %20s : ", "read");
        _debug_print_queue_stats(hlt_thread_queue_stats_reader(queue));
        fprintf(stderr, "  %20s : %lu   queue size: %lu  batches pending: %lu\n", "blocked jobs", thread->num_blocked_jobs, hlt_thread_queue_size(thread->jobs), size);
        for ( int j = 0; j < mgr->num_workers + 1; j++ ) {
            fprintf(stderr, "  %20s[%d] : ", (j==0 ? "writer-main" : "writer-worker"), j);
            _debug_print_queue_stats(hlt_thread_queue_stats_writer(queue, j));
        }
    }
            fprintf(stderr, "\n");
}

static void _debug_adapt_thread_name(hlt_worker_thread* thread)
{
    // Show queue size in top.
    uint64_t size = hlt_thread_queue_size(thread->jobs);
    char name_buffer[128];
    snprintf(name_buffer, sizeof(name_buffer), "%s (%lu)", thread->name, size);
    name_buffer[sizeof(name_buffer)-1] = '\0';
    prctl(PR_SET_NAME, name_buffer, 0, 0, 0);
}
#endif

void __hlt_thread_mgr_unblock(hlt_thread_mgr_blockable *resource, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM, "unblocking blocklable %p (#%d)", resource, resource->num_blocked);

    hlt_worker_thread* thread = ctx->worker;

    // TODO: Need better data structure.
    for ( hlt_job* job = thread->blocked_head; job; job = job->next ) {
        if ( job->func->blockable == resource ) {
            DBG_LOG(DBG_STREAM, "unblocking job %lu for vid %d in %s", job->id, job->vid, ctx->worker->name);
            _remove_from_blocked(thread, job);
            job->func->blockable = 0;
            _worker_schedule_job(thread, job, ctx);
        }
    }

    assert(resource->num_blocked == 0);
}

// Called when a virtual thread executes a ``yield`` statement. The first two
// arguments will always be 0. The contexts ``resume`` field will be set to a
// continuation to continue with when resuming.
static void _worker_yield(void* frame, void* eoss, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM, "vid %d is yielding", ctx->vid);
    _worker_schedule(ctx->worker, ctx->vid, ctx->resume, ctx->tcontext, ctx);
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
    DBG_LOG(DBG_STREAM, "executing job %lu with context %p and thread context %p", job->id, ctx, job->tcontext);

    hlt_exception* excpt = 0;

    ctx->tcontext = job->tcontext;
    __hlt_thread_mgr_run_callable(job->func, &excpt, ctx);
    ctx->tcontext = 0;

    DBG_LOG(DBG_STREAM, "done with job %lu", job->id);
}

void __hlt_thread_mgr_uncaught_exception_in_thread(hlt_exception* excpt, hlt_execution_context* ctx)
{
    hlt_worker_thread* thread = ctx->worker;

    DBG_LOG(DBG_STREAM, "uncaught exception in worker thread");

    hlt_exception_print_uncaught_in_thread(excpt, ctx);

    // We only increase, and only check whether >0 later, so this should be
    // fine without locking.
    ++thread->mgr->num_excpts;

    // We shutdown all threads.
    DBG_LOG(DBG_STREAM, "canceling all threads");
    _kill_all_threads(thread->mgr);
}

// Entry function for the worker threads.
static void* _worker(void* worker_thread_ptr)
{
    hlt_worker_thread* thread = (hlt_worker_thread*) worker_thread_ptr;
    hlt_thread_mgr* mgr = thread->mgr;

    __hlt_thread_mgr_init_native_thread(mgr, thread->name, thread->id);

    DBG_LOG(DBG_STREAM, "processing started");

    int finished = 0;

#ifdef DEBUG
    int cnt = 0;
#endif

    while ( ! hlt_thread_queue_terminated(thread->jobs) ) {
        // Process next job.
        hlt_job* job = hlt_thread_queue_read(thread->jobs, 10);

        if ( mgr->state == HLT_THREAD_MGR_FINISH ) {
            // If the manager wants to finish once everybody is idle, check
            // whether we are idle. But even if, make sure we get whatever is
            // still queued *from* us to the other workers.
            if ( ! job ) {
                if ( ! thread->idle ) {
                    for ( int i = 0; i < mgr->num_workers; ++i )
                        hlt_thread_queue_flush(mgr->workers[i]->jobs, thread->id);
                }
                thread->idle = 1;
            }
            else
                thread->idle = 0;

        }

        if ( job ) {
            if ( ! job->func->blockable )
                _worker_run_job(thread, job);
            else
                // Move to blocked queue.
                _add_to_blocked(thread, job);
        }

        if ( mgr->state == HLT_THREAD_MGR_STOP && ! finished ) {
            DBG_LOG(DBG_STREAM, "wrapping up job processing");

            // Tell all our writer ends that we're done.
            for ( int i = 0; i < mgr->num_workers; ++i )
                hlt_thread_queue_terminate_writer(mgr->workers[i]->jobs, thread->id);

            finished = 1;
        }

#ifdef DEBUG
        uint64_t size = hlt_thread_queue_size(thread->jobs);

        ++cnt;

        if ( thread->id == 1 && cnt % 5000 == 0 )
            _debug_print_job_summary(mgr);

        if ( cnt % 1000 == 0 )
            _debug_adapt_thread_name(thread);
#endif
    }

    // Signal the command queue that we're done.
    __hlt_cmd_worker_terminating(thread->id);

    DBG_LOG(DBG_STREAM, "exiting worker thread");
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

    hlt_thread_mgr_start(__hlt_global_thread_mgr);

    __hlt_cmd_queue_start();
    __hlt_files_start();
    __hlt_hooks_start();
}

void hlt_threading_stop(hlt_exception** excpt)
{
    if ( ! __hlt_global_thread_mgr )
        return;

    hlt_thread_mgr* mgr = __hlt_global_thread_mgr;

    if ( mgr->state != HLT_THREAD_MGR_DEAD ) {
        DBG_LOG(DBG_STREAM, "stopping threading system");

        assert( mgr->state == HLT_THREAD_MGR_RUN );
        // Threads are still running so shut them down. If we don't have any
        // exceptions, we give them a chance to shutdown gracefully,
        // otherwise we just kill the threads.
        hlt_thread_mgr_set_state(mgr, *excpt ? HLT_THREAD_MGR_KILL : HLT_THREAD_MGR_STOP);
    }

    assert(mgr->state == HLT_THREAD_MGR_DEAD);

    // Now that all threads have terminated, we can check for any uncaught
    // exceptions and then delete the thread manager.
    if ( ! *excpt && mgr->num_excpts != 0 ) {
        DBG_LOG(DBG_STREAM, "raising UncaughtThreadException");
        hlt_set_exception(excpt, &hlt_exception_uncaught_thread_exception, 0);
    }

    __hlt_hooks_stop();
    __hlt_files_stop();
    __hlt_cmd_queue_stop();

    hlt_thread_mgr_delete(__hlt_global_thread_mgr);
    __hlt_global_thread_mgr = 0;

    DBG_LOG(DBG_STREAM, "threading system terminated");
}

double hlt_threading_load(hlt_exception** excpt)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return 0.0;
    }

    hlt_thread_mgr* mgr = __hlt_global_thread_mgr;
    uint64_t pending = 0;

    // We count the total number of jobs pending right now, and map that
    // rather arbitrarily into a range from 0..1 (but depending on our batch
    // size).
    const double high_mark = QUEUE_MAX_LOAD; // Corresponds to 1.0

    for ( int i = 0; i < mgr->num_workers; i++ )
        pending += hlt_thread_queue_size(mgr->workers[i]->jobs);

    double load = (double)pending / high_mark;
    return load <= 1 ? load : 1;
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

    mgr->state = HLT_THREAD_MGR_NEW;
    mgr->num_workers = num;
    mgr->num_excpts = 0;
    mgr->cmdqueue = 0;

    mgr->workers = hlt_gc_malloc_non_atomic(sizeof(hlt_worker_thread*) * num);
    if ( ! mgr->workers )
        _fatal_error("cannot allocate memory for worker threads");

    return mgr;
}

void hlt_thread_mgr_start(hlt_thread_mgr* mgr)
{
    assert(mgr->state == HLT_THREAD_MGR_NEW);

    if ( pthread_key_create(&mgr->id, 0) != 0 )
        _fatal_error("cannot create thread-local key");

    __hlt_thread_mgr_init_native_thread(mgr, "main-thread", 0);

    DBG_LOG(DBG_STREAM, "found %d CPUs", hlt_util_number_of_cpus());

    mgr->state = HLT_THREAD_MGR_RUN;

    // Initialize the worker threads.
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, hlt_config_get()->stack_size);

    for ( int i = 0 ; i < mgr->num_workers; i++ ) {
        hlt_worker_thread* thread = hlt_gc_malloc_non_atomic(sizeof(hlt_worker_thread));
        thread->mgr = mgr;
        // We must not give a size limit for the queue here as otherwise the
        // scheduler will deadlock when blocking because each thread is both
        // reader and writer.
        thread->jobs = hlt_thread_queue_new(hlt_config_get()->num_workers + 1, 5000, 0);
        thread->ctxs = hlt_gc_calloc_non_atomic(3, sizeof(hlt_execution_context*));
        thread->max_vid = 2;
        thread->id = i + 1; // We leave zero for the main thread so that we can use that as its writer id.
        thread->idle = 0;
        thread->blocked_head = 0;
        thread->blocked_tail = 0;
        thread->num_blocked_jobs = 0;

        char* name = (char*) malloc(20);
        snprintf(name, 20, "worker-%d", thread->id);
        thread->name = name;

        mgr->workers[i] = thread;

        if ( pthread_create(&thread->handle, &attr, _worker, (void*) thread) != 0 ) {
            fprintf(stderr, "cannot create worker %d: %s\n", i+1, strerror(errno));
            _fatal_error("cannot create worker threads");
        }

        DBG_LOG(DBG_STREAM, "created thread for %s", thread->name);
    }

    pthread_attr_destroy(&attr);
}

void hlt_thread_mgr_delete(hlt_thread_mgr* mgr)
{
    DBG_LOG(DBG_STREAM, "deleteing");

    if ( mgr->state != HLT_THREAD_MGR_DEAD )
        _fatal_error("thread manager being deleteed still has running threads");

    if ( pthread_key_delete(mgr->id) != 0 )
        _fatal_error("cannot delete thread-local key");
}

void hlt_thread_mgr_set_state(hlt_thread_mgr* mgr, const hlt_thread_mgr_state new_state)
{
    // If we're not making a change, do nothing.
    if ( mgr->state == new_state )
        return;

    if ( mgr->state == HLT_THREAD_MGR_DEAD )
        _fatal_error("hlt_set_thread_mgr_state already dead");

    DBG_LOG(DBG_STREAM, "transitioning from mgr state %d to %d", mgr->state, new_state);

    _terminate_threads(mgr, new_state);

    mgr->state = HLT_THREAD_MGR_DEAD;

    DBG_LOG(DBG_STREAM, "mgr now in state dead, all threads terminated");
}

hlt_thread_mgr_state hlt_thread_mgr_get_state(const hlt_thread_mgr* mgr)
{
    return mgr->state;
}

uint32_t hlt_thread_mgr_num_threads(hlt_thread_mgr* mgr)
{
    return mgr->num_workers;
}

void __hlt_thread_mgr_schedule(hlt_thread_mgr* mgr, hlt_vthread_id vid, hlt_continuation* func, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return;
    }

    hlt_worker_thread* thread = _vthread_to_worker(mgr, vid);
    _worker_schedule(thread, vid, func, 0, ctx);
}

void __hlt_thread_mgr_schedule_tcontext(hlt_thread_mgr* mgr, const struct __hlt_type_info* type, void* tcontext, struct hlt_continuation* func, struct hlt_exception** excpt, struct hlt_execution_context* ctx)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return;
    }

    hlt_vthread_id vid = tcontext ? (*type->hash)(type, &tcontext, 0, 0) : 0;

    if ( vid < 0 )
        vid = -vid;

    // Scale the VID into the right interval.
    const hlt_config* cfg = hlt_config_get();
    hlt_vthread_id n = (cfg->vid_schedule_max - cfg->vid_schedule_min + 1);

    if ( n <= 0 )
        _fatal_error("invalid config.vld_schedule_{min,max} values");

    hlt_vthread_id scaled_vid = (vid % n) + cfg->vid_schedule_min;

    hlt_worker_thread* thread = _vthread_to_worker(mgr, scaled_vid);
    _worker_schedule(thread, scaled_vid, func, tcontext, ctx);
}

const char* hlt_thread_mgr_current_native_thread(hlt_thread_mgr* mgr)
{
    if ( ! mgr )
        return "no threading";

    const char* id = pthread_getspecific(mgr->id);
    return id ? id : "<no thread id>";
}

void __hlt_thread_mgr_init_native_thread(hlt_thread_mgr* mgr, const char* name, int default_affinity)
{
    if ( ! mgr )
        return;

    pthread_t self = pthread_self();

    prctl(PR_SET_NAME, name, 0, 0, 0);
    if ( pthread_setspecific(mgr->id, name) != 0 )
        _fatal_error("cannot set thread-local key");

    DBG_LOG(DBG_STREAM, "native thread %p assigned name '%s'", self, name);

    // Set core affinity if specified for this thread.
    int core = -1;
    const char* p = hlt_config_get()->core_affinity;

    if ( strcmp(p, "DEFAULT") == 0 ) // Magic configuration string.
        core = default_affinity % hlt_util_number_of_cpus();

    else {
        p = p ? strstr(p, name) : 0;

        if ( p ) {
            const char *end = p + strlen(name);
            if ( *end == ':' )
                core = atoi(++end);
        }
    }

    if ( core >= 0 ) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core, &cpuset);

        // Set affinity.
        if ( pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset) != 0 ) {
            fprintf(stderr, "cannot set affinity for thread %s to core %d: %s\n", name, core, strerror(errno));
            _fatal_error("affinity error");
        }

        // Check that it worked.
        if ( pthread_getaffinity_np(self, sizeof(cpu_set_t), &cpuset) != 0 )
            _fatal_error("can't get thread affinity");

        if ( ! CPU_ISSET(core, &cpuset) ) {
            fprintf(stderr, "setting affinity for thread %s to core %d did not work\n", name, core);
            _fatal_error("affinity error");
        }

        DBG_LOG(DBG_STREAM, "native thread %p pinned to core %d", self, core);
    }
}

