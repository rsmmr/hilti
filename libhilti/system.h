///
/// Encapsulates platform-specific code.
///

#include <pthread.h>

#ifdef DARWIN
// We don't have pthread spinlocks on DARWIN.
# define PTHREAD_SPINLOCK_T      pthread_mutex_t
# define PTHREAD_SPIN_LOCK(x)    pthread_mutex_lock(x)
# define PTHREAD_SPIN_UNLOCK(x)  pthread_mutex_unlock(x)
# define PTHREAD_SPIN_INIT(x)    pthread_mutex_init(x, 0)
# define PTHREAD_SPIN_DESTROY(x) pthread_mutex_destroy(x)
#else
# define PTHREAD_SPINLOCK_T      pthread_spinlock_t
# define PTHREAD_SPIN_LOCK(x)    pthread_spin_lock(x)
# define PTHREAD_SPIN_UNLOCK(x)  pthread_spin_unlock(x)
# define PTHREAD_SPIN_INIT(x)    pthread_spin_init(x, PTHREAD_PROCESS_PRIVATE)
# define PTHREAD_SPIN_DESTROY(x) pthread_spin_destroy(x)
#endif

/// Sets a name for the vurrently running thread, as far as supported by the
/// OS.
void hlt_set_thread_name(const char* s);

