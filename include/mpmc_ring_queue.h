/*
 * mpmc_ring_queue.h - lock-free, non-blocking message MPMC queue (based on fixed size ring buffer).
 * 
 * This implementation is based on the bounded MPMC queue proposed by Dmitry Vyukov
 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
  * 
 * Producers and consumers operate independently, but the operations are not
 * lock-free, i.e., a enqueue may have to wait for a pending dequeue
 * operation to finish and vice versa.
  */

#ifndef MPMC_RING_QUEUE_H
#define MPMC_RING_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <queuedef.h>

typedef struct mpmc_q_s mpmc_ring_queue; /* object type of queue instance */


/* Create an instance of a ring queue.
 * rtn_q  : Pointer to caller's queue instance handle.
 * q_size : Number of queue elements to allocate.  Must be > 1 and a power
 *          of 2.  Due to the nature of the algorithm used, a maximum of
 *          q_size - 1 elements can actually be stored in the queue.
 * Returns QERR_OK on success, or other QERR_* value on error. */
mpmc_ring_queue *mpmc_ring_queue_new(size_t q_size, qerr_t *qerr);

/* Delete an instance of a queue (items not freed, call mpmc_ring_queue_delete in this case).
 * q : Queue instance handle.
 * Returns QERR_OK on success, or other QERR_* value on error. */
qerr_t mpmc_ring_queue_destroy(mpmc_ring_queue *q);

/* Delete an instance of a queue (call mpmc_ring_queue_destroy) and free all items before.
 * q : Queue instance handle.
 * Returns QERR_OK on success, or other QERR_* value on error. */
qerr_t mpmc_ring_queue_delete(mpmc_ring_queue *q);

/* Add a message to the queue.
 * q : Queue instance handle.
 * m : Message to enqueue (don't pass NULL).
 * Returns QERR_OK on success, QERR_FULL if queue full, or other QERR_* value on
 * error. */
qerr_t mpmc_ring_queue_enqueue(mpmc_ring_queue *q, void *m);

/* Remove a message from the queue.
 * q     : Queue instance handle.
 * Returns dequeued message or NULL if queue empthy */
void *mpmc_ring_queue_dequeue(mpmc_ring_queue *q);

/* Check if queue is empthy.
 * q : Queue instance handle. */
int mpmc_ring_queue_empty(mpmc_ring_queue *q);

/* Check if queue is full.
 * q : Queue instance handle. */
int mpmc_ring_queue_full(mpmc_ring_queue *q);

/* Return queue capacity.
 * q : Queue instance handle. */
size_t mpmc_ring_queue_size(mpmc_ring_queue *q);

/* Return queue free len. Not full consistent.
 * Also can return 0 if queue is full or capacity if queue is empthy due to paralell enqueue/dequeue.
 * q : Queue instance handle. */
size_t mpmc_ring_queue_free_relaxed(mpmc_ring_queue *q);

#ifdef __cplusplus
}
#endif

#endif /* MPMC_RING_QUEUE_H */
