// $Id$

#include <stdlib.h>

#include "module.h"

void hilti_abort(hlt_exception** excpt)
{
    abort();
}

// A non-yielding sleep.
void hilti_sleep(double secs, hlt_exception** excpt)
{
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) ((secs - sleep_time.tv_sec) * 1e9);
    
    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;
}

void hilti_wait_for_threads()
{
    hlt_thread_mgr_set_state(__hlt_global_thread_mgr, HLT_THREAD_MGR_FINISH);
}

int64_t hilti_bytes_to_int(hlt_bytes* b, hlt_exception** excpt)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt);
    int8_t* tmp = hlt_gc_malloc_atomic(len);
    int8_t* p = tmp;
    
    hlt_bytes_pos cur = hlt_bytes_begin(b, excpt);
    hlt_bytes_pos end = hlt_bytes_end(excpt);

    int64_t value = 0;
    int8_t first = 1;
    int8_t negate = 0;
    
    while ( ! hlt_bytes_pos_eq(cur, end, excpt) ) {
        int8_t ch = __hlt_bytes_extract_one(&cur, end, excpt);
        
        if ( isdigit(ch) )
            value = (value * 10) + (ch - '0');
        
        else if ( first && ch == '-' )
            negate = 1;
        
        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }
    
        first = 0;
    }
    
    return negate ? -value : value; 
}

hlt_bytes* hilti_bytes_to_lower(hlt_bytes* b, hlt_exception** excpt)
{
    hlt_bytes_size len = hlt_bytes_len(b, excpt);
    int8_t* tmp = hlt_gc_malloc_atomic(len);
    int8_t* p = tmp;
    
    hlt_bytes_pos cur = hlt_bytes_begin(b, excpt);
    hlt_bytes_pos end = hlt_bytes_end(excpt);
    
    while ( ! hlt_bytes_pos_eq(cur, end, excpt) )
        *p++ = tolower(__hlt_bytes_extract_one(&cur, end, excpt));
    
    return hlt_bytes_new_from_data(tmp, len, excpt);
}

int8_t hilti_bytes_starts_with(hlt_bytes* b, hlt_bytes* s, hlt_exception** excpt)
{
    hlt_bytes_size len_b = hlt_bytes_len(b, excpt);
    hlt_bytes_size len_s = hlt_bytes_len(s, excpt);
    
    if ( len_s > len_b )
        return 0;
    
    hlt_bytes_pos cur_b = hlt_bytes_begin(b, excpt);
    hlt_bytes_pos cur_s = hlt_bytes_begin(s, excpt);
    hlt_bytes_pos end = hlt_bytes_end(excpt);
    
    while ( ! hlt_bytes_pos_eq(cur_s, end, excpt) ) {
        int8_t c1 = __hlt_bytes_extract_one(&cur_b, end, excpt);
        int8_t c2 = __hlt_bytes_extract_one(&cur_s, end, excpt);
        if ( c1 != c2 )
            return 0;
    }
    
    return 1;
}
