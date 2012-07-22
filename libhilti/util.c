
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>

#include "util.h"
#include "globals.h"
#include "rtti.h"
#include "globals.h"
#include "threading.h"
#include "globals.h"

void hlt_util_nanosleep(uint64_t nsecs)
{
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) (nsecs / 1e9);
    sleep_time.tv_nsec = (nsecs - sleep_time.tv_sec * 1e9);
    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;
}

static void _reverse(char* start, char* end)
{
    char tmp;

    while( end > start ) {
        tmp = *end;
        *end-- = *start;
        *start++ = tmp;
    }
}

int hlt_util_uitoa_n(uint64_t value, char* buf, int n, int base, int zerofill)
	{
	static char dig[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	assert(buf && n > 0);

	int i = 0;

	do {
		buf[i++] = dig[value % base];
		value /= base;
	} while ( value && i < n - 1 );

    while ( zerofill && i < n - 1 )
        buf[i++] = '0';

	buf[i] = '\0';

    _reverse(buf, buf + i - 1);

	return i + 1;
	}

int hlt_util_number_of_cpus()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

void hlt_pthread_setcancelstate(int state, int *oldstate)
{
    if ( ! hlt_global_thread_mgr() )
        return;

    if ( hlt_global_thread_mgr()->state != HLT_THREAD_MGR_RUN )
        pthread_setcancelstate(state, oldstate);
}

size_t hlt_util_memory_usage()
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return (r.ru_maxrss * 1024);
}

static inline uint64_t _flip(uint64_t v)
{
    union {
        uint64_t ui64;
        unsigned char c[8];
    } x;

    char c;

    x.ui64 = v;
    c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
    c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
    c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
    c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;

    return x.ui64;
}

uint64_t hlt_hton64(uint64_t v)
{
#if ! __BIG_ENDIAN__
    return _flip(v);
#else
    return v;
#endif
}

uint32_t hlt_hton32(uint32_t v)
{
    return ntohl(v);
}

uint16_t hlt_hton16(uint16_t v)
{
    return ntohs(v);
}

uint64_t hlt_ntoh64(uint64_t v)
{
#if ! __BIG_ENDIAN__
    return _flip(v);
#else
    return v;
#endif
}

uint32_t hlt_ntoh32(uint32_t v)
{
    return ntohl(v);
}

uint16_t hlt_ntoh16(uint16_t v)
{
    return ntohs(v);
}

void hlt_abort()
{
    __hlt_debug_print("hilti-mem", "hlt_abort() called");
    fprintf(stderr, "internal HILTI error: hlt_abort() called");
    abort();
}

hlt_hash hlt_hash_bytes(const int8_t *s, int16_t len)
{
    // This is copied and adapted from hhash.h
    if ( ! len )
        return 0;

	hlt_hash h = 0;
    while ( len-- )
        h = (h << 5) - h + *s++;

	return h;
}

hlt_hash hlt_default_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_hash hash = hlt_hash_bytes(obj, type->size);
    return hash;
}

int8_t hlt_default_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return memcmp(obj1, obj2, type1->size) == 0;
}

int8_t safe_write(int fd, const char* data, int len)
{
	while ( len > 0 ) {
		int n = write(fd, data, len);

		if ( n < 0 ) {
			if ( errno == EINTR )
				continue;

			return 0;
        }

		data += n;
		len -= n;
    }

	return 1;
}
