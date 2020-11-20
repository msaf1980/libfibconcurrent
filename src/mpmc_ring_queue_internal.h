#ifndef MPMC_RING_QUEUE_INTERNAL_H
#define MPMC_RING_QUEUE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arch.h"

/* message element */
struct q_msg_s {
    volatile void *d;
    volatile size_t seq;
};
typedef struct q_msg_s q_msg_t;

/* queue.h contains an empty forward definition of "q_s", and defines "queue" */
struct mpmc_q_s {
    size_t put_pos; /* next put position (tail pointer) */
    char _put_pad[CACHE_LINE_SIZE -
                 (sizeof(size_t))]; /* align next var on cache line */

    size_t get_pos; /* first message for dequeue (head pointer) */
    char _get_pad[CACHE_LINE_SIZE -
                 (sizeof(size_t))]; /* align next var on cache line */

    q_msg_t * volatile msgs; /* Array of elements of q_msg_t */
    size_t capacity;    /* Number of max msgs elements */
    size_t capacity_mod;    /* Number of max msgs elements minus 1 */
    /* make total size a multiple of cache line size, to prevent interference
     * with whatever comes after */
    char _final_pad[CACHE_LINE_SIZE - (2 * sizeof(size_t) + sizeof(void **))];
}; /* struct mpmc_q_s */

#ifdef __cplusplus
}
#endif

#endif /* MPMC_RING_QUEUE_INTERNAL_H */
