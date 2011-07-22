
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "util.h"
#include "globals.h"
#include "threading.h"

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
    if (  __hlt_global_thread_mgr->state != HLT_THREAD_MGR_RUN )
        pthread_setcancelstate(state, oldstate);
}
