
#include <pthread.h>

#include "system.h"

#ifdef HAVE_LINUX
#include <sys/prctl.h>
#endif

void hlt_set_thread_name(const char* s)
{
#ifdef HAVE_LINUX
	prctl(PR_SET_NAME, s, 0, 0, 0);
#endif

#ifdef DARWIN
	pthread_setname_np(s);
#endif

#ifdef FREEBSD
	pthread_set_name_np(pthread_self(), s, s);
#endif
}

