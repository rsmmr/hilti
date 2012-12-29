// Profiling support.
//
// TODO: Not everything declared is alreadt implemented. We are missing currently:
//
//     Counters:        heap, alloced.
//     Snapshot types:  wall, cycles.
//
// TODO: The PAPI profiling is performed on native threads, not HILTI virtual
// threads. That will mess the numbers up quite a bit when used with a
// threaded HILTI program, however it's unclear whether that can be fixed.
//
// TODO: We don't have 64-bit ntohl() yet, so we store the profiles just in
// host format.

#include <fcntl.h>
#include <errno.h>

#include "profiler.h"
#include "globals.h"
#include "debug.h"
#include "hutil.h"
#include "utf8proc.h"
#include "string_.h"
#include "timer.h"
#include "autogen/hilti-hlt.h"

typedef hlt_hash khint_t;
#include "3rdparty/khash/khash.h"

#ifdef HAVE_PAPI
#include <papi.h>

#define PAPI_NUM_EVENTS 2

#endif

typedef struct {
    hlt_string tag;           // The hash tag.
    hlt_timer_mgr* tmgr;      // Timer manager attached, or NULL.
    hlt_timer* timer;         // Snapshot timer if installed, or NULL. Not memory-managed to avoid cycles.
    hlt_enum style;           // The profile style.
    uint64_t param;           // Parameter for the profile style.
    uint64_t level;           // Depth of nested calls currently.

    uint64_t time;            // Timer mgr time at beginning.
    uint64_t wall;            // Wall time at beginning.
    uint64_t updates;         // Number of update calls so far.
    uint64_t cycles;          // Cycle counter at beginning.
    uint64_t cache;           // Cache state at beginning.
    uint64_t heap;            // Heap size at beginning.
    uint64_t user;            // Value of user counter currently.
} __hlt_profiler;

typedef struct __kh_table_t {
    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    hlt_string *keys;
    __hlt_profiler **vals;
} kh_table_t;

struct __hlt_profiler_state {
    kh_table_t *profilers;     // Active profilers.
    int fd;                    // Output file.
};

typedef struct kh_hlt_profiler_table_t kh_hlt_profiler_table_t;

static inline hlt_hash __kh_string_hash_func(hlt_string tag, const hlt_type_info* type)
{
    return hlt_string_hash(&hlt_type_info_hlt_string, &tag, 0, 0);
}

static inline int8_t __kh_string_equal_func(hlt_string tag1, hlt_string tag2, const hlt_type_info* type)
{
    return hlt_string_equal(&hlt_type_info_hlt_string, &tag1, &hlt_type_info_hlt_string, &tag2, 0, 0);
}

KHASH_INIT(table, hlt_string, __hlt_profiler*, 1, __kh_string_hash_func, __kh_string_equal_func)

#ifdef HAVE_PAPI

void init_papi()
{
    DBG_LOG("hilti-profiler", "PAPI: initializing library");

	if ( PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
        DBG_LOG("hilti-profiler", "PAPI: cannot initialize library (unsupported platform? lacking perms?)");
        goto error;
    }

    // TODO: Can we pass in here a function that return the vid?
	int ret = 0; // FIXME PAPI_thread_init(pthread_self);
    if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot init thread support, %s", PAPI_strerror(ret));
        goto error;
    }

    if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot create event set, %s", PAPI_strerror(ret));
        goto error;
		}

    ret = PAPI_create_eventset(&__hlt_globals()->papi_set);

    if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot create event set, %s", PAPI_strerror(ret));
        goto error;
    }

    // Note: Increase PAPI_NUM_EVENTS if adding more events.
    ret = PAPI_add_event(__hlt_globals()->papi_set, PAPI_TOT_CYC);
	if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot add PAPI_TOT_CYC to event set, %s", PAPI_strerror(ret));
        goto error;
    }

    ret = PAPI_add_event(__hlt_globals()->papi_set, PAPI_L1_DCM);
	if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot add PAPI_L1_DCM to event set, %s", PAPI_strerror(ret));
        goto error;
    }

	PAPI_option_t options;
	memset(&options, 0, sizeof(options));
	options.domain.eventset = __hlt_globals()->papi_set;
	options.domain.domain = PAPI_DOM_ALL;

    ret = PAPI_set_opt(PAPI_DOMAIN, &options);

	if ( ret != PAPI_OK ) {
        DBG_LOG("hilti-profiler", "PAPI: cannot set options, %s", PAPI_strerror(ret));
        goto error;
		}

    if ( (ret = PAPI_start(__hlt_globals()->papi_set)) != PAPI_OK) {
        DBG_LOG("hilti-profiler", "PAPI: cannot start counters, %s", PAPI_strerror(ret));
        goto error;
		}

    __hlt_globals()->papi_available = 1;
    return;

error:
    __hlt_globals()->papi_available = 0;
    }

void read_papi(long_long* cnts)
{
    if ( ! __hlt_globals()->papi_available )
        goto error;

    int ret = PAPI_read(__hlt_globals()->papi_set, cnts);

    if ( ret == PAPI_OK)
        return;

    DBG_LOG("hilti-profiler", "PAPI: cannot read counters, %s", PAPI_strerror(ret));

error:
    for ( int i = 0; i < PAPI_NUM_EVENTS; i++ )
        cnts[i] = 0;
}

#endif

inline static void _safe_write(const void* data, int len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(ctx->pstate->fd >= 0);

    while ( len ) {
        int written = write(ctx->pstate->fd, data, len);

        if ( written < 0 ) {
            if ( errno == EAGAIN || errno == EINTR )
                continue;

            char buffer[128];
            strerror_r(errno, buffer, sizeof(buffer));
            hlt_string err = hlt_string_from_asciiz(buffer, excpt, ctx);
            hlt_set_exception(excpt, &hlt_exception_io_error, err);
            return;
        }

        data += written;
        len -= written;
    }
}

inline static int _safe_read(int fd, void* data, int len)
{
    while ( len ) {
        int nread = read(fd, data, len);

        if ( nread == 0 )
            return 0; // Eof.

        if ( nread < 0 ) {

            if ( errno == EAGAIN || errno == EINTR )
                continue;

            return -1;
        }

        data += nread;
        len -= nread;
    }

    return 1;
}

inline static void write_tag(hlt_string str, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int8_t len = str->len;
    _safe_write(&len, sizeof(len), excpt, ctx);
    // We write this out in UTF8, and decode when reading.
    _safe_write(&str->bytes, len, excpt, ctx);
}

inline static int read_tag(int fd, char* tag)
{
    int8_t len = 0;

    int ret = _safe_read(fd, &len, sizeof(len));
    if ( ret <= 0 )
        return ret;

    char buffer[len];

    if ( _safe_read(fd, &buffer, len) <= 0 )
        return -1; // Eof is an error here.

    char* p = buffer;
    char* e = buffer + len;

    while ( p < e ) {
        int32_t uc;
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &uc);

        if ( n < 0 ) {
            fprintf(stderr, "HILTI profiling: cannot decode UTF8 character\n");
            return 0;
        }

        *tag++ = uc < 128 && isprint(uc) ? uc : '?';
        p += n;
    }

    *tag = '\0';

    return 1;
}

static const char* MAGIC = "HLTPROF";

static void write_header(hlt_exception** excpt, hlt_execution_context* ctx)
{
    _safe_write(MAGIC, sizeof(MAGIC) - 1, excpt, ctx);

    uint64_t version = hlt_hton64(HLT_PROFILER_VERSION);
    _safe_write(&version, sizeof(version), excpt, ctx );

    time_t t = time(0);
    uint64_t secs = hlt_hton64(t);
    _safe_write(&secs, sizeof(secs), excpt, ctx);
}

static int read_header(int fd, time_t* t)
{
    char buffer[sizeof(MAGIC) - 1];
    if ( _safe_read(fd, buffer, sizeof(MAGIC) - 1) <= 0 )
        return -1; // Eof is an error here.

    if ( memcmp(MAGIC, buffer, sizeof(MAGIC) - 1) != 0 ) {
        fprintf(stderr, "HILTI profiling: file format not recognized when reading profile\n");
        return -1;
    }

    uint64_t version = 0;
    if ( _safe_read(fd, &version, sizeof(version)) <= 0 )
        return -1; // Eof is an error here.

    version = hlt_ntoh64(version);
    if ( version != HLT_PROFILER_VERSION ) {
        fprintf(stderr, "HILTI profiling: wrong version when reading profile\n");
        return -1;
    }

    uint64_t secs = 0;

    if ( _safe_read(fd, &secs, sizeof(version)) <= 0 )
        return -1; // Eof is an error here.

    if ( t )
        *t = hlt_ntoh64(secs);

    return 1;
}

static void write_record(int8_t rtype, __hlt_profiler* p, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_profiler_record rec;

    uint64_t cwall = hlt_time_wall(excpt, ctx);
    uint64_t ctime = p->tmgr ? hlt_timer_mgr_current(p->tmgr, excpt, ctx) : 0;

    rec.ctime = hlt_hton64(ctime);
    rec.cwall = hlt_hton64(cwall);
    rec.time = hlt_hton64(p->tmgr ? ctime - p->time : 0);
    rec.wall = hlt_hton64(cwall - p->wall);

    rec.updates = hlt_hton64(p->updates);
    rec.alloced = hlt_hton64(hlt_util_memory_usage() - p->heap);
    rec.heap = hlt_hton64(hlt_util_memory_usage());
    rec.user = hlt_hton64(p->user);

#ifdef HAVE_PAPI
    long_long cnts[PAPI_NUM_EVENTS];
    read_papi(cnts);
    rec.cycles = (rtype != HLT_PROFILER_START) ? hlt_hton64(cnts[0] - p->cycles) : 0;
    rec.misses = (rtype != HLT_PROFILER_START) ? hlt_hton64(cnts[1] - p->cache) : 0;
#else
    rec.cycles = hlt_hton64(0);
    rec.misses = hlt_hton64(0);
#endif

    rec.type = rtype;

    write_tag(p->tag, excpt, ctx);
    _safe_write(&rec, sizeof(rec), excpt, ctx);
}

static int read_record(int fd, char* tag, hlt_profiler_record* rec)
{
    int ret = read_tag(fd, tag);

    if ( ret <= 0 )
        return ret;

    if ( _safe_read(fd, rec, sizeof(hlt_profiler_record)) <= 0 )
        return -1; // Eof is an error here.

    rec->ctime = hlt_ntoh64(rec->ctime);
    rec->cwall = hlt_ntoh64(rec->cwall);
    rec->time = hlt_ntoh64(rec->time);
    rec->wall = hlt_ntoh64(rec->wall);
    rec->updates = hlt_ntoh64(rec->updates);;
    rec->cycles = hlt_ntoh64(rec->cycles);
    rec->misses = hlt_ntoh64(rec->misses);
    rec->alloced = hlt_ntoh64(rec->alloced);
    rec->heap = hlt_ntoh64(rec->heap);
    rec->user = hlt_ntoh64(rec->user);

    return 1;
}

static void output_record(__hlt_profiler* p, int8_t record_type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ctx->pstate->fd < 0 ) {
        // Output file not yet opened.
        char buffer[128];
        if ( ctx->vid >= 0 )
            snprintf(buffer, sizeof(buffer), "hlt.prof.p%d.t%" PRId64 ".dat", getpid(), ctx->vid);
        else
            snprintf(buffer, sizeof(buffer), "hlt.prof.p%d.dat", getpid());

        int fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, 0770);

        if ( fd < 0 ) {
            strerror_r(errno, buffer, sizeof(buffer));
            hlt_string err = hlt_string_from_asciiz(buffer, excpt, ctx);
            hlt_set_exception(excpt, &hlt_exception_io_error, err);
            return;
        }

        ctx->pstate->fd = fd;

        write_header(excpt, ctx);

        if ( *excpt )
            return;
        }

    write_record(record_type, p, excpt, ctx);
}

static void install_timer(__hlt_profiler* p, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(! p->timer);
    assert(p->tmgr);

    p->timer = __hlt_timer_new_profiler(p->tag, excpt, ctx);
    hlt_time t = (hlt_timer_mgr_current(p->tmgr, excpt, ctx) / p->param ) * p->param + p->param;

    hlt_timer_mgr_schedule(p->tmgr, t, p->timer, excpt, ctx);
    GC_DTOR(p->timer, hlt_timer); // Not memory-managed on our end.
}

void __hlt_profiler_state_delete(__hlt_profiler_state* state)
{
    for ( khiter_t i = kh_begin(state->profilers); i != kh_end(state->profilers); i++ ) {
        if ( kh_exist(state->profilers, i) )
            hlt_free(kh_value(state->profilers, i));
    }

    kh_destroy_table(state->profilers);
    hlt_free(state->profilers);

    close(state->fd);
    hlt_free(state);
}

void hlt_profiler_timer_expire(hlt_string tag, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(ctx->pstate->profilers);

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    assert(i != kh_end(ctx->pstate->profilers));

    __hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

    output_record(p, HLT_PROFILER_SNAPSHOT, excpt, ctx);

    // Reinstall the timer.
    p->timer = 0;
    install_timer(p, excpt, ctx);
}

void __hlt_profiler_init()
{
    __hlt_globals()->profiling_enabled = hlt_config_get()->profiling;

#ifdef HAVE_PAPI
    if ( __hlt_globals()->profiling_enabled )
        init_papi();
#endif
}

void __hlt_profiler_done()
{
    __hlt_globals()->profiling_enabled = 0;
}

void hlt_profiler_start(hlt_string tag, hlt_enum style, uint64_t param, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __hlt_globals()->profiling_enabled )
        return;

    if ( ! ctx->pstate ) {
        ctx->pstate = hlt_malloc(sizeof(__hlt_profiler_state));
        ctx->pstate->profilers = kh_init(table);
        ctx->pstate->fd = -1;
    }

    __hlt_profiler* p = 0;

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i != kh_end(ctx->pstate->profilers) )
        p = kh_value(ctx->pstate->profilers, i);

    if ( ! p ) {

        if ( tag->len > HLT_PROFILER_MAX_TAG_LENGTH - 1 ) {
            // We keep the tags short enough to store their length in a
            // single byte. Note that we really want the *raw* length here.
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        // We don't know this profiler yet.
        __hlt_profiler* p = hlt_calloc(1, sizeof(__hlt_profiler));
        p->tag = tag;
        p->tmgr = tmgr;
        p->timer = 0;
        p->style = style;
        p->param = param;

        p->time = p->tmgr ? hlt_timer_mgr_current(p->tmgr, excpt, ctx) : 0;
        p->wall = hlt_time_wall(excpt, ctx);
        p->heap = hlt_util_memory_usage();

#ifdef HAVE_PAPI
        long_long cnts[PAPI_NUM_EVENTS];
        read_papi(cnts);
        p->cycles = hlt_hton64(cnts[0]);
        p->cache = hlt_hton64(cnts[1]);
#else
        p->cycles = hlt_hton64(0);
        p->cache = hlt_hton64(0);
#endif

        p->level = 1;
        p->updates = 0;
        p->user = 0;

        int ret;
        i = kh_put_table(ctx->pstate->profilers, tag, &ret, 0);
        kh_value(ctx->pstate->profilers, i) = p;

        if ( hlt_enum_equal(style, Hilti_ProfileStyle_Time, excpt, ctx) ) {
            assert(tmgr);
            install_timer(p, excpt, ctx);
        }

        output_record(p, HLT_PROFILER_START, excpt, ctx);
    }

    else {
        // Increase level for existing profiler. We make sure it exists and
        // that we don't get conflicting arguments. If we do, we throw an
        // exception.
        __hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

        if ( ! p ) {
            hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
            return;
        }

        if ( ! hlt_enum_equal(style, p->style, excpt, ctx) ||
             param != p->param ||
             tmgr != p->tmgr ) {
            hlt_set_exception(excpt, &hlt_exception_profiler_mismatch, 0);
            return;
        }

        ++p->level;
    }
}

void hlt_profiler_update(hlt_string tag, uint64_t user_delta, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __hlt_globals()->profiling_enabled )
        return;

    if ( ! ctx->pstate->profilers ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i == kh_end(ctx->pstate->profilers) ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    __hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

    if ( ! p ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    ++p->updates;
    p->user += user_delta;

    int do_record = 1;

    // Check if snapshot condition has been reached.
    if ( hlt_enum_equal(p->style, Hilti_ProfileStyle_Standard, excpt, ctx) )
        ; // Nothing to do.

    else if ( hlt_enum_equal(p->style, Hilti_ProfileStyle_Time, excpt, ctx) )
        ; // Nothing to do here.

    else if ( hlt_enum_equal(p->style, Hilti_ProfileStyle_Updates, excpt, ctx) ) {
        if ( (p->updates % p->param) == 0 )
            output_record(p, HLT_PROFILER_SNAPSHOT, excpt, ctx);

        // We suppress normal updates here, as most likely the caller wants
        // *only* the snapshots with this style.
        do_record = 0;
    }

    else
        hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);

    if ( do_record )
        output_record(p, HLT_PROFILER_UPDATE, excpt, ctx);

}

void hlt_profiler_stop(hlt_string tag, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __hlt_globals()->profiling_enabled )
        return;

    if ( ! ctx->pstate->profilers ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i == kh_end(ctx->pstate->profilers) ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    __hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

    if ( ! p ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, tag);
        return;
    }

    if ( --p->level == 0 ) {
        // Done with this profiler.
        output_record(p, HLT_PROFILER_STOP, excpt, ctx);

        if ( p->timer ) {
            hlt_timer_cancel(p->timer, excpt, ctx);
            p->timer = 0;
        }

        hlt_free(p);
        kh_value(ctx->pstate->profilers, i) = 0;
    }
}

int hlt_profiler_file_open(const char* fname, time_t* t)
{
    int fd = open(fname, O_RDONLY);
    if ( fd < 0 )
        return -1;

    if ( ! read_header(fd, t) )
        return -1;

    return fd;
}

int hlt_profiler_file_read(int fd, char* tag, hlt_profiler_record* record)
{
    return read_record(fd, tag, record);
}

void hlt_profiler_file_close(int fd)
{
    close(fd);
}
