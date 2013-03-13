
#include <stdlib.h>
#include <ctype.h>

#include "module.h"
#include "../globals.h"

void hilti_abort(hlt_exception** excpt)
{
    fprintf(stderr, "Hilti::abort() called.");
    abort();
}

void hilti_terminate(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_termination, 0);
}

// A non-yielding sleep.
void hilti_sleep(double secs, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_thread_mgr* mgr = hlt_global_thread_mgr();

    // Flush our job queues so that the stuff can get processes.
    for ( int i = 0; i < mgr->num_workers; ++i )
        hlt_thread_queue_flush(mgr->workers[i]->jobs, 0);

    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) ((secs - sleep_time.tv_sec) * 1e9);

    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;
}

void hilti_wait_for_threads()
{
    hlt_thread_mgr_set_state(hlt_global_thread_mgr(), HLT_THREAD_MGR_FINISH);
}

int64_t hilti_bytes_to_int(hlt_bytes* b, int64_t base, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    int64_t value = 0;
    int64_t t = 0;
    int8_t first = 1;
    int8_t negate = 0;

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {
        int8_t ch = __hlt_bytes_extract_one(&cur, end, excpt, ctx);

        if ( isdigit(ch) )
            t = ch - '0';
        else if ( isalpha(ch) )
            t = 10 + tolower(ch) - 'a';

        else if ( first && ch == '-' )
            negate = 1;

        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            GC_DTOR(cur, hlt_iterator_bytes);
            GC_DTOR(end, hlt_iterator_bytes);
            return 0;
        }

        value = (value * base) + t;
        first = 0;
    }

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return negate ? -value : value;
}

hlt_bytes* hilti_bytes_to_lower(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    int8_t* tmp = hlt_malloc(len);
    int8_t* p = tmp;

    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) )
        *p++ = tolower(__hlt_bytes_extract_one(&cur, end, excpt, ctx));

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return hlt_bytes_new_from_data(tmp, len, excpt, ctx);
}

hlt_bytes* hilti_bytes_to_upper(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    int8_t* tmp = hlt_malloc(len);
    int8_t* p = tmp;

    hlt_iterator_bytes cur = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);

    while ( ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) )
        *p++ = toupper(__hlt_bytes_extract_one(&cur, end, excpt, ctx));

    GC_DTOR(cur, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return hlt_bytes_new_from_data(tmp, len, excpt, ctx);
}

int8_t hilti_bytes_starts_with(hlt_bytes* b, hlt_bytes* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_size len_b = hlt_bytes_len(b, excpt, ctx);
    hlt_bytes_size len_s = hlt_bytes_len(s, excpt, ctx);

    if ( len_s > len_b )
        return 0;

    hlt_iterator_bytes cur_b = hlt_bytes_begin(b, excpt, ctx);
    hlt_iterator_bytes cur_s = hlt_bytes_begin(s, excpt, ctx);
    hlt_iterator_bytes end_b = hlt_bytes_end(b, excpt, ctx);
    hlt_iterator_bytes end_s = hlt_bytes_end(s, excpt, ctx);

    int result = 1;

    while ( ! hlt_iterator_bytes_eq(cur_s, end_s, excpt, ctx) ) {
        int8_t c1 = __hlt_bytes_extract_one(&cur_b, end_b, excpt, ctx);
        int8_t c2 = __hlt_bytes_extract_one(&cur_s, end_s, excpt, ctx);
        if ( c1 != c2 ) {
            result = 0;
            break;
        }
    }

    GC_DTOR(cur_b, hlt_iterator_bytes);
    GC_DTOR(cur_s, hlt_iterator_bytes);
    GC_DTOR(end_b, hlt_iterator_bytes);
    GC_DTOR(end_s, hlt_iterator_bytes);

    return result;
}
