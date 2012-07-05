///
/// HILTI's threading support, including the run-time scheduler.
///

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#define DBG_STREAM       "hilti-threads"
#define DBG_STREAM_STATS "hilti-threads-stats"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/prctl.h>
#include <pthread.h>

#include "threading.h"
#include "debug.h"
#include "config.h"
#include "callable.h"
#include "context.h"
#include "globals.h"
#include "exceptions.h"

typedef hlt_hash khint_t;
#include "3rdparty/khash/khash.h"

typedef struct __hlt_blocked_job {
    hlt_job* job;
    struct __hlt_blocked_job* next;
} hlt_blocked_job;

typedef struct __kh_blocked_jobs_t {
    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    const void** keys;
    hlt_blocked_job** vals;
} kh_blocked_jobs_t;

static inline hlt_hash __kh_ptr_hash_func(const void* p, const void* unused)
{
    return (hlt_hash)p;
}

static inline int8_t __kh_ptr_equal_func(const void* p1, const void* p2, const void* unused)
{
    return p1 == p2;
}

KHASH_INIT(blocked_jobs, const void*, hlt_blocked_job*, 1, __kh_ptr_hash_func, __kh_ptr_equal_func)

// Batch size for the jobs queues.
#define QUEUE_BATCH_SIZE 5000

// Number of pending elements across all job qeueus that correspond to the
// maximum load of 1.0.
#define QUEUE_MAX_LOAD   (QUEUE_BATCH_SIZE * 3 * mgr->num_workers)

#ifdef DEBUG
uint64_t job_counter = 0;
#endif

static void _fatal_error(const char* msg)
{
    fprintf(stderr, "libhilti threading: %s\n", msg);
    exit(1);
}

static void _hlt_job_delete(hlt_job* j)
{
    // TODO;
}

static void _hlt_worker_thread_delete(hlt_worker_thread* t)
{
    // TODO;
}

static void _hlt_blocked_job_free(hlt_blocked_job* job)
{
    // TODO;
}

void hlt_thread_mgr_delete(hlt_thread_mgr* mgr)
{
    DBG_LOG(DBG_STREAM, "deleteing");

    if ( mgr->state != HLT_THREAD_MGR_DEAD )
        _fatal_error("thread manager being deleteed still has running threads");

    if ( pthread_key_delete(mgr->id) != 0 )
        _fatal_error("cannot delete thread-local key");
}

int8_t __hlt_thread_mgr_terminating()
{
    return __hlt_global_thread_mgr_terminate;
}

// Flag all threads to terminate immediately. Safe to call from all threads.
void _kill_all_threads(hlt_thread_mgr* mgr)
{
    DBG_LOG(DBG_STREAM, "terminating all threads");
    __hlt_global_thread_mgr_terminate = 1;
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
    _kill_all_threads(thread->mgr);
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
    DBG_LOG(DBG_STREAM, "terminating all threads (state %d)", state);

    mgr->state = state;

    // Terminate the main thread's write ends of the job queues.
    for ( int i = 0; i < mgr->num_workers; ++i )
        hlt_thread_queue_terminate_writer(mgr->workers[i]->jobs, 0);

    switch ( state ) {
      case HLT_THREAD_MGR_FINISH:
        DBG_LOG(DBG_STREAM, "waiting for all threads to become idle");

        // This loop is not free of race conditions. However, I claim that
        // the cases where it's unpredicatable are ill-defined to begin with! :)
        while ( 1 ) {
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
        // Workers will check for this state.
        break;

      case HLT_THREAD_MGR_KILL:
        _kill_all_threads(mgr);
        break;

      default:
        _fatal_error("internal error: unknown state in terminate threads");
    }

    DBG_LOG(DBG_STREAM, "joining workers");

    // Join all the workers.
    for ( i = 0; i < mgr->num_workers; i++ ) {
        DBG_LOG(DBG_STREAM, "joining %s", mgr->workers[i]->name);
        if ( pthread_join(mgr->workers[i]->handle, 0) != 0 )
            _fatal_error("cannot join worker thread");
    }

    DBG_LOG(DBG_STREAM, "all workers joined");
}

static void _add_to_blocked(hlt_worker_thread* thread, __hlt_thread_mgr_blockable* resource, hlt_job* job)
{
    DBG_LOG(DBG_STREAM, "added job %lu to blocked queue for %s with blockable %p", job->id, thread->name, job->blockable);

    hlt_blocked_job* bjob = hlt_malloc(sizeof(hlt_blocked_job));
    bjob->job = job;

    khiter_t i = kh_get_blocked_jobs(thread->jobs_blocked, resource, 0);

    if ( i == kh_end(thread->jobs_blocked) ) {
        int ret;
        i = kh_put_blocked_jobs(thread->jobs_blocked, resource, &ret, 0);
        bjob->next = 0;
    }

    else
        bjob->next = kh_value(thread->jobs_blocked, i);

    kh_value(thread->jobs_blocked, i) = bjob;

    ++resource->num_blocked;
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

        thread->ctxs = hlt_realloc(thread->ctxs, (new_max+1) * sizeof(hlt_execution_context*));
        memset(thread->ctxs + max + 1, 0, (new_max - max) * sizeof(hlt_execution_context*));

        thread->max_vid = new_max;
    }

    hlt_execution_context* ctx = thread->ctxs[vid];

    if ( ! ctx ) {
        // Haven't seen this thread yet, need to create new context.
        ctx = __hlt_execution_context_new(vid);
        ctx->worker = thread;
        thread->ctxs[vid] = ctx;
    }

    return ctx;
}

// The top-level function to run inside a job's fiber. The received argument is the callable to execute.
static void _worker_fiber_entry(hlt_fiber* fiber, void *callable)
{
    hlt_execution_context* ctx = hlt_fiber_context(fiber);
    hlt_exception* excpt = 0;
    hlt_callable_run((hlt_callable *)callable, &excpt, ctx);

    if ( excpt )
        __hlt_thread_mgr_uncaught_exception_in_thread(excpt, ctx);

    hlt_fiber_return(fiber);
}

// Schedules a job to a worker thread.
static void _worker_schedule_job(hlt_worker_thread* target, hlt_job* job)
{
    DBG_LOG(DBG_STREAM, "scheduling job %lu for vid %d to %s", job->id, job->vid, target->name);

    if ( ! job->blockable )
        hlt_thread_queue_write(target->jobs, target ? target->id : 0, job);
    else
        _add_to_blocked(target, job->blockable, job);
}

static void _unblock_blocked(hlt_worker_thread* thread, __hlt_thread_mgr_blockable* resource, hlt_execution_context* ctx)
{
    khiter_t i = kh_get_blocked_jobs(thread->jobs_blocked, resource, 0);

    // hlt_thread_mgr_unblock() only calls us if there's a blocked job.
    assert(i != kh_end(thread->jobs_blocked));

    DBG_LOG(DBG_STREAM, "unblocking resource %p in %s", resource, ctx->worker->name);

    for ( hlt_blocked_job* bjob = kh_value(thread->jobs_blocked, i); bjob; bjob = bjob->next ) {
        hlt_job* job = bjob->job;
        DBG_LOG(DBG_STREAM, "removing job %lu from blocked queue for %s", job->id, thread->name);
        assert(job->blockable == resource);
        job->blockable = 0;
        --resource->num_blocked;
        _worker_schedule_job(thread, job);
    }

    kh_value(thread->jobs_blocked, i) = 0;
    kh_del_blocked_jobs(thread->jobs_blocked, i);
}

static void _worker_schedule(hlt_worker_thread* target, hlt_vthread_id vid, hlt_callable* func, const hlt_type_info* tcontext_type, void* tcontext, hlt_execution_context* ctx)
{
    if ( target->mgr->state != HLT_THREAD_MGR_RUN && target->mgr->state != HLT_THREAD_MGR_FINISH ) {
        DBG_LOG(DBG_STREAM, "omitting scheduling of job because mgr signaled termination");
        return;
    }

    hlt_job* job = hlt_malloc(sizeof(hlt_job));
    job->fiber = hlt_fiber_create(_worker_fiber_entry, _worker_get_ctx(target, vid), func);
    job->vid = vid;
    job->tcontext_type = tcontext_type;
    job->tcontext = tcontext;
    job->next = 0;
    job->prev = 0;
#if DEBUG
    job->id = ++job_counter;
#endif

    GC_CCTOR_GENERIC(&job->tcontext, tcontext_type);

    _worker_schedule_job(target, job);
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
        fprintf(stderr, "  %20s : %lu   queue size: %lu  batches pending: %lu\n", "blocked jobs", kh_size(thread->jobs_blocked), hlt_thread_queue_size(thread->jobs), size);
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

void __hlt_thread_mgr_unblock(__hlt_thread_mgr_blockable *resource, hlt_execution_context* ctx)
{
    DBG_LOG(DBG_STREAM, "unblocking blocklable %p (#%d)", resource, resource->num_blocked);

    hlt_worker_thread* thread = ctx->worker;
    _unblock_blocked(thread, resource, ctx);
}

static void _worker_run_job(hlt_worker_thread* thread, hlt_job* job)
{
    assert(job->fiber);

    hlt_execution_context* ctx = hlt_fiber_context(job->fiber);

    DBG_LOG(DBG_STREAM, "executing job %lu with context %p and thread context %p", job->id, ctx, job->tcontext);

    hlt_exception* excpt = 0;

    ctx->fiber = job->fiber;
    ctx->tcontext = job->tcontext;
    ctx->tcontext_type = job->tcontext_type;

    GC_CCTOR_GENERIC(&ctx->tcontext, ctx->tcontext_type);

    if ( hlt_fiber_start(job->fiber) == 0 ) {
        // Yield.
        DBG_LOG(DBG_STREAM, "vid %d is yielding", ctx->vid);
        _worker_schedule_job(ctx->worker, job);
    }

    else {
        // Done with this.
        DBG_LOG(DBG_STREAM, "done with job %lu", job->id);

        hlt_fiber_delete(ctx->fiber);

        GC_DTOR_GENERIC(&ctx->tcontext, ctx->tcontext_type);
        ctx->fiber = 0;
        ctx->tcontext = 0;
    }
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

    while ( ! (__hlt_thread_mgr_terminating() || hlt_thread_queue_terminated(thread->jobs)) ) {
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
            if ( ! job->blockable )
                _worker_run_job(thread, job);
            else
                // Move to blocked queue.
                _add_to_blocked(thread, job->blockable, job);
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
    return hlt_config_get()->num_workers != 0;
}

void __hlt_threading_init()
{
    if ( hlt_config_get()->num_workers == 0 ) {
        DBG_LOG(DBG_STREAM, "no worker threads configured");
        // No threading configured.
        return;
    }

    DBG_LOG(DBG_STREAM, "initializing threading system");

    hlt_thread_mgr* mgr = hlt_thread_mgr_new();

    if ( ! mgr )
        _fatal_error("cannot create threading manager");

    __hlt_global_set_thread_mgr(mgr);

    hlt_thread_mgr_start(mgr);
}

void __hlt_threading_done(hlt_exception** excpt)
{
    if ( ! hlt_global_thread_mgr() )
        return;

    hlt_thread_mgr* mgr = hlt_global_thread_mgr();

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

    hlt_thread_mgr_delete(hlt_global_thread_mgr());
    __hlt_global_set_thread_mgr(0);

    DBG_LOG(DBG_STREAM, "threading system terminated");
}

double hlt_threading_load(hlt_exception** excpt)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return 0.0;
    }

    hlt_thread_mgr* mgr = hlt_global_thread_mgr();
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
    hlt_thread_mgr* mgr = hlt_malloc(sizeof(hlt_thread_mgr));

    int num = hlt_config_get()->num_workers;
    if ( ! num )
        _fatal_error("no worker threads configured");

    mgr->state = HLT_THREAD_MGR_NEW;
    mgr->num_workers = num;
    mgr->num_excpts = 0;
    mgr->workers = hlt_malloc(sizeof(hlt_worker_thread*) * num);

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
        hlt_worker_thread* thread = hlt_malloc(sizeof(hlt_worker_thread));
        thread->mgr = mgr;
        // We must not give a size limit for the queue here as otherwise the
        // scheduler will deadlock when blocking because each thread is both
        // reader and writer.
        thread->jobs = hlt_thread_queue_new(hlt_config_get()->num_workers + 1, 5000, 0, 1);
        thread->ctxs = hlt_calloc(3, sizeof(hlt_execution_context*));
        thread->max_vid = 2;
        thread->id = i + 1; // We leave zero for the main thread so that we can use that as its writer id.
        thread->idle = 0;
        thread->jobs_blocked = kh_init(blocked_jobs);

        char* name = (char*) hlt_malloc(20);
        snprintf(name, 20, "worker-%d", thread->id);
        thread->name = name;

        mgr->workers[i] = thread;

        if ( pthread_create(&thread->handle, &attr, _worker, (void*) thread) != 0 ) {
            fprintf(stderr, "cannot create worker %d: %s\n", i+1, strerror(errno));
            _fatal_error("cannot create worker threads");
        }

        DBG_LOG(DBG_STREAM, "created thread %p for %s", thread->handle, thread->name);
    }

    pthread_attr_destroy(&attr);
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

void __hlt_thread_mgr_schedule(hlt_thread_mgr* mgr, hlt_vthread_id vid, hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! hlt_is_multi_threaded() ) {
        hlt_set_exception(excpt, &hlt_exception_no_threading, 0);
        return;
    }

    hlt_worker_thread* thread = _vthread_to_worker(mgr, vid);
    _worker_schedule(thread, vid, func, 0, 0, ctx);
}

void __hlt_thread_mgr_schedule_tcontext(hlt_thread_mgr* mgr, const struct __hlt_type_info* type, void* tcontext, hlt_callable* func, hlt_exception** excpt, hlt_execution_context* ctx)
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
    _worker_schedule(thread, scaled_vid, func, type, tcontext, ctx);
}

const char* hlt_thread_mgr_current_native_thread()
{
    if ( ! hlt_global_thread_mgr() )
        return "<no threading>";

    const char* id = pthread_getspecific(hlt_global_thread_mgr()->id);
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
        (void)CPU_SET(core, &cpuset); // Cast to get around compiler warning.

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

