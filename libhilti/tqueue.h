///
/// An internal data structure implementing a thread-safe
/// multiple-writer-single-reader queue.  We guarantee in-order delivery for
/// each writer but not across writers. We enumerate all writer threads, and
/// each write operation must specify which thread is doing the write.

#ifndef HILTI_TQUEUE_H
#define HILTI_TQUEUE_H

#include <stdint.h>

typedef struct __hlt_thread_queue hlt_thread_queue;

/// Creates a new thread-safe multiple-writer-single-reader queue.
///
/// writers: Number of concurrent writer to support.
///
/// batch_size: Number of elements that will be buffered on the writer side
/// before being passed on to the reader side.
///
/// max_batches: The maximum number of batches that can be pending between
/// writers and reader, waiting for the latter to pick it up. If a queue is
/// full, further queuing will block. 0 disables any limit and guarantees
/// that queuing will not block. Note that the limit is less than the number
/// of batches that may still be unprocessed; it doesn't count whatever the
/// reader has already grabbed some but not yet processed.
///
/// Returns: The new queue.
hlt_thread_queue* hlt_thread_queue_new(int writers, int batch_size, int max_batches);

/// Releases all acquired resources.
///
/// queue: The queue to delete.
void hlt_thread_queue_delete(hlt_thread_queue* queue);

/// Writes an element into the queue. The write may block iff the queue's
/// size limit is reached.
///
/// queue: The queue into which to write.
///
/// writer: The writer thread doing the write in the range between zero and
/// *writers*-1, as passsed to ~~hlt_thread_queue_new. Note that different
/// threads must not use the same writer number.
///
/// elem: The element to write into the queue. The queue just stores the
/// pointer.
void hlt_thread_queue_write(hlt_thread_queue* queue, int writer, void *elem);

/// Reads an element from the queue. This function must only be called from a
/// single thread. Note that it may not return an element even if there are 
/// still unprocessed ones pending. Call ~~flush if you want to be sure to
/// get everything.
///
/// queue: The queue from which to read.
///
/// timeout: Number of microseconds that the read should wait for an element
/// to become available. If zero, the read will block until one becomes
/// available or all writers have signaled termination. If -1, the read will
/// return immediately if no element is available.
//
/// Returns: The element or NULL if no available.
void* hlt_thread_queue_read(hlt_thread_queue* queue, int timeout);

/// Flushes the writer side for the given writer thread. All elements this
/// writer has already written will be made available immediately to the
/// reader. However, the flush may block iff the queue's size limit is
/// reached.
//
/// This method is relatively expensive as it will always acquire the queue's
/// lock.
///
/// queue: The queue from which to read.
///
/// writer: The writer thread doing the flush in the range between zero and
/// *writers*-1, as passsed to ~~hlt_thread_queue_new. Note that different
/// threads must not use the same writer number. Only the given writer's
/// elements are flushed.
void hlt_thread_queue_flush(hlt_thread_queue* queue, int writer);

/// Returns an approximation of the queue's current size. This may be called
/// by both reader and writer threads but the returned number is not
/// guaranteed to be correct.
///
/// queue: The queue from which to read.
///
/// Returns: A guess at how many elements are currently queued.
uint64_t hlt_thread_queue_size(hlt_thread_queue* queue);

/// Returns true if there's at least one element ready for reading. Must only
/// be called by the reader thread. If it returns true, the next call to
/// ~~hlt_thread_queue_read is guaranteed to return an element without
/// blocking. Note however that it may return false even if some elements are
/// still pending if the reader hasn't picked those up yet. And even if it
/// returns false, the next read may succeed.
///
/// queue: The queue from which to read.
///
/// Returns: True if an element is ready.
int8_t hlt_thread_queue_can_read(hlt_thread_queue* queue);

/// Terminates a writer. Must only be called from the corresponding thread.
/// Subsequent writes from that writer will be ignored.
///
/// queue: The queue from which to read.
///
/// writer: The writer thread that's terminating.
void hlt_thread_queue_terminate_writer(hlt_thread_queue* queue, int writer);

/// Returns true if ~~hlt_thread_queue_terminate_writer() has been called by
/// all writers *and* all queued entries have been read.
///
/// queue: The queue for which to check the termination status.
///
/// Returns: True if done.
int8_t hlt_thread_queue_terminated(hlt_thread_queue* queue);

extern uint64_t hlt_thread_queue_pending(hlt_thread_queue* queue);

typedef struct {
    uint64_t elems;
    uint64_t batches;
    uint64_t blocked;
    uint64_t locked;
} hlt_thread_queue_stats;

const hlt_thread_queue_stats* hlt_thread_queue_stats_reader(hlt_thread_queue* queue);
const hlt_thread_queue_stats* hlt_thread_queue_stats_writer(hlt_thread_queue* queue, int writer);

#endif
