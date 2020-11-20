#include <stdlib.h>

#include <stdbool.h>

#include "mpmc_ring_queue_internal.h"
#include <mpmc_ring_queue.h>

#define QERR(p_qerr, qerr_n)                                                   \
    do {                                                                       \
        if (p_qerr != NULL)                                                    \
            *p_qerr = qerr_n;                                                  \
    } while (0)

mpmc_ring_queue *mpmc_ring_queue_new(size_t q_size, qerr_t *qerr) {
    size_t i;
    qerr_t ret;
    int perr;
    mpmc_ring_queue *q = NULL;

    if (sizeof(mpmc_ring_queue) % CACHE_LINE_SIZE != 0) {
        QERR(qerr, QERR_BUG2);
        return NULL;
    } /* mpmc_ring_queue not multiple of cache line size */

    if ((ret = queue_size_is_valid(q_size)) != QERR_OK) {
        QERR(qerr, ret);
        return NULL;
    }

    /* Create queue object instance */
    perr = posix_memalign((void **) &q, CACHE_LINE_SIZE, sizeof(*q));
    if (perr != 0 || q == NULL) {
        QERR(qerr, QERR_MALLOCERR);
        return NULL;
    }

    /* Allocate message storage array */
    q->msgs = NULL;
    perr = posix_memalign((void **) &q->msgs, CACHE_LINE_SIZE,
                          q_size * sizeof(q->msgs[0]));
    if (perr != 0 || q->msgs == NULL) {
        free(q);
        QERR(qerr, QERR_MALLOCERR);
        return NULL;
    }

    /* empty the queue */
    for (i = 0; i < q_size; i++) {
        q->msgs[i].seq = i;
        q->msgs[i].d = NULL;
    }

    q->put_pos = 0; /* Init the queue counters */
    q->get_pos = 0;

    q->capacity = q_size;
    /* bit mask to "and" enq_cnt and deq_cnt to get tail and head */
    q->capacity_mod = q_size - 1;

    /* Success */
    return q;
} /* mpmc_ring_queue_new */

qerr_t mpmc_ring_queue_destroy(mpmc_ring_queue *q) {
    free((void *) q->msgs);
    q->msgs = NULL;
    free(q);

    return QERR_OK;
} /* mpmc_ring_queue_destroy */

qerr_t mpmc_ring_queue_delete(mpmc_ring_queue *q) {
    void *p;
    while ((p = mpmc_ring_queue_dequeue(q)) != NULL) {
        free((char *) p);
    }

    return mpmc_ring_queue_destroy(q);
} /* mpmc_ring_queue_delete */

qerr_t mpmc_ring_queue_enqueue(mpmc_ring_queue *q, void *m) {
    q_msg_t *msg;
    size_t put_pos, seq;
    ssize_t dif;

    put_pos = __atomic_load_n(&q->put_pos, __ATOMIC_RELAXED);
    for (;;) {
        msg = &q->msgs[put_pos & q->capacity_mod];
        // this acquire-load synchronizes-with the release-store (2)
        seq = __atomic_load_n(&msg->seq, __ATOMIC_ACQUIRE);
        dif = (ssize_t) seq - (ssize_t) put_pos;
        if (dif == 0) {
            if (__atomic_compare_exchange_n(&q->put_pos, &put_pos, put_pos + 1,
                                            true, __ATOMIC_RELAXED,
                                            __ATOMIC_RELAXED)) {
                break;
            }
        } else if (dif < 0) {
            return QERR_FULL;
        } else {
            put_pos = __atomic_load_n(&q->put_pos, __ATOMIC_RELAXED);
        }
    }
    msg->d = m;
    __atomic_store_n(&msg->seq, put_pos + 1, __ATOMIC_RELEASE);
    return QERR_OK;
} /* mpmc_ring_queue_enqueue */

void *mpmc_ring_queue_dequeue(mpmc_ring_queue *q) {
    size_t get_pos, seq, next;
    ssize_t dif;
    void *m;
    q_msg_t *msg;

    get_pos = __atomic_load_n(&q->get_pos, __ATOMIC_RELAXED);
    for (;;) {
        next = get_pos + 1;
        msg = &q->msgs[get_pos & q->capacity_mod];
        seq = __atomic_load_n(&msg->seq, __ATOMIC_ACQUIRE);
        dif = (ssize_t) seq - (ssize_t) next;
        if (dif == 0) {
            if (__atomic_compare_exchange_n(&q->get_pos, &get_pos, next, true,
                                            __ATOMIC_RELAXED,
                                            __ATOMIC_RELAXED)) {
                break;
            }
        } else if (dif < 0) {
            return NULL;
        } else {
            get_pos = __atomic_load_n(&q->get_pos, __ATOMIC_RELAXED);
        }
    }
    m = (void *) msg->d;
    __atomic_store_n(&msg->seq, next + q->capacity_mod, __ATOMIC_RELEASE);
    return m;
} /* mpmc_ring_queue_dequeue */

inline size_t mpmc_ring_queue_size(mpmc_ring_queue *q) {
    return q->capacity;
} /* queue_size */

int mpmc_ring_queue_empty(mpmc_ring_queue *q) {
    size_t get_pos = __atomic_load_n(&q->get_pos, __ATOMIC_RELAXED);
    q_msg_t *msg = &q->msgs[get_pos & q->capacity_mod];
    size_t seq = __atomic_load_n(&q->msgs[get_pos & q->capacity_mod].seq,
                                 __ATOMIC_RELAXED);
    return !(seq == get_pos + 1);
}

int mpmc_ring_queue_full(mpmc_ring_queue *q) {
    size_t put_pos = __atomic_load_n(&q->put_pos, __ATOMIC_RELAXED);
    q_msg_t *msg = &q->msgs[put_pos & q->capacity_mod];
    size_t seq = __atomic_load_n(&q->msgs[put_pos & q->capacity_mod].seq,
                                 __ATOMIC_RELAXED);
    return (seq != put_pos);
}

size_t mpmc_ring_queue_free_relaxed(mpmc_ring_queue *q) {
    size_t get_pos, put_pos;
    put_pos = __atomic_load_n(&q->put_pos, __ATOMIC_RELAXED);
    get_pos = __atomic_load_n(&q->get_pos, __ATOMIC_ACQUIRE);
    if (get_pos == put_pos) {
        q_msg_t *msg = &q->msgs[get_pos & q->capacity_mod];
        if (__atomic_load_n(&msg->seq, __ATOMIC_RELAXED) == get_pos) {
            return mpmc_ring_queue_size(q);
        } else {
            return 0;
        }
    }
    return mpmc_ring_queue_size(q) - (put_pos - get_pos);
}