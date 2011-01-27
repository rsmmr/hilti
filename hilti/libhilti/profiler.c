// Profiling support.
//
// TODO: Not everything declared is alreadt implemented. We are missing currently:
//
//     Counters:        cycles, cache, heap, alloced.
//     Snapshot types:  wall, cycles.
//
//
//  We don't have 64-bit ntohl() yet, so we store the profiles just in host format. 

#include "hilti.h"
#include "utf8proc.h"

typedef hlt_hash khint_t;
#include "3rdparty/khash/khash.h"

#include <endian.h>

static inline uint64_t ntohll(uint64_t x)
{
    // FIXME.
    return x;
}

static inline uint64_t htonll(uint64_t x)
{
    // FIXME.
    return x;
}

#include <fcntl.h>
#include <errno.h>

typedef struct {
    hlt_string tag;           // The hash tag.
    hlt_timer_mgr* tmgr;      // Timer manager attached, or NULL.
    hlt_timer* timer;         // Snapshot timer if installed, or NULL.
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
} hlt_profiler;

typedef struct __kh_table_t {
    // These are used by khash and copied from there (see README.HILTI).
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    hlt_string *keys;
    hlt_profiler **vals;
} kh_table_t;

struct hlt_profiling_state {
    kh_table_t *profilers;     // Active profilers.
    int fd;                   // Output file.
};

typedef struct kh_hlt_profiler_table_t kh_hlt_profiler_table_t;

extern const hlt_type_info hlt_type_info_string;

static inline hlt_hash __kh_string_hash_func(hlt_string tag, const hlt_type_info* type)
{
    return hlt_string_hash(&hlt_type_info_string, &tag, 0, 0);
}

static inline int8_t __kh_string_equal_func(hlt_string tag1, hlt_string tag2, const hlt_type_info* type)
{
    return hlt_string_equal(&hlt_type_info_string, &tag1, &hlt_type_info_string, &tag2, 0, 0);
}

KHASH_INIT(table, hlt_string, hlt_profiler*, 1, __kh_string_hash_func, __kh_string_equal_func)

inline static void safe_write(const void* data, int len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(ctx->pstate->fd >= 0);

    while ( len ) {
        int written = write(ctx->pstate->fd, data, len);

        if ( written < 0 ) {
            if ( errno == EAGAIN || errno == EINTR )
                continue;

            char buffer[128];
            char* msg = strerror_r(errno, buffer, sizeof(buffer));
            hlt_string err = hlt_string_from_asciiz(msg, excpt, ctx);
            hlt_set_exception(excpt, &hlt_exception_io_error, err);
            return;
        }

        data += written;
        len -= written;
    }
}

inline static int safe_read(int fd, const void* data, int len)
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
    safe_write(&len, sizeof(len), excpt, ctx);
    // We write this out in UTF8, and decode when reading.
    safe_write(&str->bytes, len, excpt, ctx);
}

inline static int read_tag(int fd, char* tag)
{
    int8_t len;
    char buffer[len];

    int ret = safe_read(fd, &len, sizeof(len));
    if ( ret <= 0 )
        return ret;

    if ( safe_read(fd, &buffer, len) <= 0 )
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

        // FIXME: Without this explicit tmp, the output get mangled. Don't
        // know why ...
        int8_t c = (int8_t) uc;

        // Replace non-ASCII characters with '?'.
        *tag++ = c < 128 && isprint(c) ? (int8_t)uc : '?';
        p += n;
    }

    *tag = '\0';

    return 1;
}

static const char* MAGIC = "HLTPROF";

static void write_header(hlt_exception** excpt, hlt_execution_context* ctx)
{
    safe_write(MAGIC, sizeof(MAGIC) - 1, excpt, ctx);

    uint64_t version = htonll(HLT_PROFILER_VERSION);
    safe_write(&version, sizeof(version), excpt, ctx );

    time_t t = time(0);
    uint64_t secs = htonll(t);
    safe_write(&secs, sizeof(secs), excpt, ctx);
}

static int read_header(int fd, time_t* t)
{
    const char buffer[sizeof(MAGIC) - 1];
    if ( safe_read(fd, buffer, sizeof(MAGIC) - 1) <= 0 )
        return -1; // Eof is an error here.

    if ( memcmp(MAGIC, buffer, sizeof(MAGIC) - 1) != 0 ) {
        fprintf(stderr, "HILTI profiling: file format not recognized when reading profile\n");
        return -1;
    }

    uint64_t version;
    if ( safe_read(fd, &version, sizeof(version)) <= 0 )
        return -1; // Eof is an error here.

    version = ntohll(version);
    if ( version != HLT_PROFILER_VERSION ) {
        fprintf(stderr, "HILTI profiling: wrong version when reading profile\n");
        return -1;
    }

    uint64_t secs;

    if ( safe_read(fd, &secs, sizeof(version)) <= 0 )
        return -1; // Eof is an error here.

    if ( t )
        *t = ntohll(secs);

    return 1;
}

static void write_record(int8_t rtype, hlt_profiler* p, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_profiler_record rec;

    rec.ctime = htonll(p->tmgr ? hlt_timer_mgr_current(p->tmgr, excpt, ctx) : 0);
    rec.cwall = htonll(hlt_time_wall(excpt, ctx));
    rec.time = htonll(p->tmgr ? rec.ctime - p->time : 0);
    rec.wall = htonll(rec.cwall - p->wall);

    rec.updates = htonll(p->updates);
    rec.cycles = htonll(0);  // XXX
    rec.misses = htonll(0);  // XXX
    rec.alloced = htonll(0); // XXX
    rec.heap = htonll(0);    // XXX
    rec.user = htonll(p->user);

    rec.type = rtype;

    write_tag(p->tag, excpt, ctx);
    safe_write(&rec, sizeof(rec), excpt, ctx);
}

static int read_record(int fd, char* tag, hlt_profiler_record* rec)
{
    int ret = read_tag(fd, tag);

    if ( ret <= 0 )
        return ret;

    if ( safe_read(fd, rec, sizeof(hlt_profiler_record)) <= 0 )
        return -1; // Eof is an error here.

    rec->ctime = ntohll(rec->ctime);
    rec->cwall = ntohll(rec->cwall);
    rec->time = ntohll(rec->time);
    rec->wall = ntohll(rec->wall);
    rec->updates = ntohll(rec->updates);;
    rec->cycles = ntohll(rec->cycles);
    rec->misses = ntohll(rec->misses);
    rec->alloced = ntohll(rec->alloced);
    rec->heap = ntohll(rec->heap);
    rec->user = ntohll(rec->user);

    return 1;
}

static void output_record(hlt_profiler* p, int8_t record_type, hlt_exception** excpt, hlt_execution_context* ctx)
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
            char* msg = strerror_r(errno, buffer, sizeof(buffer));
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

static void install_timer(hlt_profiler* p, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(! p->timer);
    assert(p->tmgr);

    p->timer = __hlt_timer_new_profiler(p->tag, excpt, ctx);
    hlt_time t = (hlt_timer_mgr_current(p->tmgr, excpt, ctx) / p->param ) * p->param + p->param;

    hlt_timer_mgr_schedule(p->tmgr, t, p->timer, excpt, ctx);
}

void hlt_profiler_timer_expire(hlt_string tag, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(ctx->pstate->profilers);

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    assert(i != kh_end(ctx->pstate->profilers));

    hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

    output_record(p, HLT_PROFILER_SNAPSHOT, excpt, ctx);

    // Reinstall the timer.
    p->timer = 0;
    install_timer(p, excpt, ctx);
}

void __hlt_profiler_init()
{
    const hlt_config *cfg = hlt_config_get();
    if ( ! cfg->profiling )
        return;

}

/*
#ifdef HAVE_PAPI

PAPISet = PAPI_NULL;
EXTERN_CC(bool, have_papi, false);

	if ( PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
		fprintf(stderr, "PAPI warning: cannot init library (not running with sufficient permissions?)\n");
        goto error;
    }

    if ( PAPI_thread_init(handle) != PAPI_OK ) {
		fprintf(stderr, "PAPI warning: cannot init thread support: %s\n", PAPI_strerror(ret));
        goto error;
    }


	int ret;
	if ( (ret = PAPI_create_eventset(&PAPISet)) != PAPI_OK )
		{

#if 0
	if ( (ret = PAPI_add_event(PAPISet, PAPI_TOT_INS)) != PAPI_OK )
		{
		fprintf(stderr, "Error in adding into the event set (tot_ins): %s\n", PAPI_strerror(ret));
		return;
		}
#endif

	if ( (ret = PAPI_add_event(PAPISet, PAPI_TOT_CYC)) != PAPI_OK )
		{
		fprintf(stderr, "Error in adding into the event set (tot_cyc): %s\n", PAPI_strerror(ret));
		return;
		}

	PAPI_option_t options;
	memset(&options, 0, sizeof(options));
	options.domain.eventset = PAPISet;
	options.domain.domain = PAPI_DOM_ALL;
	if ( (ret = PAPI_set_opt(PAPI_DOMAIN, &options)) != PAPI_OK )
		{
		fprintf(stderr, "Error in setting PAPI domain: %s\n", PAPI_strerror(ret));
		return;
		}

	if ( (ret = PAPI_start(PAPISet)) != PAPI_OK)
		{
		fprintf(stderr, "Error in starting PAPI counters: %s\n", PAPI_strerror(ret));
		return;
		}

	have_papi = true;
	}
#endif
}
*/

void hlt_profiler_start(hlt_string tag, hlt_enum style, uint64_t param, hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! ctx->pstate ) {
        ctx->pstate = hlt_gc_malloc_non_atomic(sizeof(hlt_profiling_state));

        if ( ! ctx->pstate ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return;
        }

        ctx->pstate->profilers = kh_init(table);

        if ( ! ctx->pstate->profilers ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return;
        }

        ctx->pstate->fd = -1;
    }

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i == kh_end(ctx->pstate->profilers) ) {

        if ( tag->len > HLT_PROFILER_MAX_TAG_LENGTH - 1 ) {
            // We keep the tags short enough to store their length in a
            // single byte. Note that we really want the *raw* length here.
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        // We don't know this profiler yet.
        hlt_profiler* p = hlt_gc_calloc_non_atomic(1, sizeof(hlt_profiler));

        if ( ! p ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return;
        }

        p->tag = tag;
        p->tmgr = tmgr;
        p->timer = 0;
        p->style = style;
        p->param = param;

        p->time = p->tmgr ? hlt_timer_mgr_current(p->tmgr, excpt, ctx) : 0;
        p->wall = hlt_time_wall(excpt, ctx);

        p->cycles = 0; // FIXME
        p->cache = 0; // FIXME
        p->heap = 0; // FIXME

        p->level = 1;
        p->updates = 0;
        p->user = 0;

        int ret;
        i = kh_put_table(ctx->pstate->profilers, tag, &ret, 0);
        assert(ret); // Can't exist yet.
        kh_value(ctx->pstate->profilers, i) = p;

        if ( hlt_enum_equal(style, Hilti_ProfileStyle_Time, excpt, ctx) ) {
            assert(tmgr);
            install_timer(p, excpt, ctx);
        }

        output_record(p, HLT_PROFILER_START, excpt, ctx);
    }

    else {
        // Increase level for existing profiler. We make sure that we don't
        // get conflicting arguments. If we do, we throw an exception.
        hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

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
    if ( ! ctx->pstate->profilers ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, 0);
        return;
    }

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i == kh_end(ctx->pstate->profilers) ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, 0);
        return;
    }

    hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

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
    if ( ! ctx->pstate->profilers ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, 0);
        return;
    }

    khiter_t i = kh_get_table(ctx->pstate->profilers, tag, 0);

    if ( i == kh_end(ctx->pstate->profilers) ) {
        hlt_set_exception(excpt, &hlt_exception_profiler_unknown, 0);
        return;
    }

    hlt_profiler* p = kh_value(ctx->pstate->profilers, i);

    if ( --p->level == 0 ) {
        // Done with this profiler.
        output_record(p, HLT_PROFILER_STOP, excpt, ctx);

        if ( p->timer ) {
            hlt_timer_cancel(p->timer, excpt, ctx);
            p->timer = 0;
        }

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
