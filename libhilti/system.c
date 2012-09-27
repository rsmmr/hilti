
#include <pthread.h>
#include <getopt.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "system.h"

void hlt_set_thread_name(const char* s)
{
#ifdef __linux__
	prctl(PR_SET_NAME, s, 0, 0, 0);
#endif

#ifdef DARWIN
	pthread_setname_np(s);
#endif

#ifdef FREEBSD
	pthread_set_name_np(pthread_self(), s, s);
#endif
}

void hlt_set_thread_affinity(int core)
{
#ifdef HAVE_LINUX
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
#endif
}

void hlt_reset_getopt()
{
    // Methods differ here, see man pages.
#ifdef __linux__
    optind = 0;
#else
    // Works on Apple, not clear it works on BSD in general ...
    optreset = 1;
    optind = 1;
#endif
}
