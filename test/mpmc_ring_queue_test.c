#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#include <mpmc_ring_queue.h>
#include <mpmc_ring_queue_internal.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

#define V2I (size_t)(void *)
#define I2V (void *) (size_t)

CTEST(mpmc_ring_queue_new, bad_size) {
    mpmc_ring_queue *q;
    qerr_t qerr;
    q = mpmc_ring_queue_new(0, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 0 must errored with QERR_BADSIZE");
    q = mpmc_ring_queue_new(1, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 1 must errored with QERR_BADSIZE");
    q = mpmc_ring_queue_new(3, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 3 must errored with QERR_BADSIZE");
    q = mpmc_ring_queue_new(5, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 5 must errored with QERR_BADSIZE");
    q = mpmc_ring_queue_new(6, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 6 must errored with QERR_BADSIZE");
    q = mpmc_ring_queue_new(7, &qerr);
    ASSERT_EQUAL_D(QERR_BADSIZE, qerr, "size 7 must errored with QERR_BADSIZE");
    ASSERT_NULL(q);
} /* CTEST(queue_new, bad_size) */

CTEST(mpmc_ring_queue_base, enqueue_dequeue) {
    mpmc_ring_queue *q, save_q;
    qerr_t qerr = QERR_OK;
    void *v;
    q = mpmc_ring_queue_new(4, &qerr);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_new must successed");
    ASSERT_NOT_NULL_D(q, "queue must be not NULL");

    /* copy for compare memory */
    save_q = *q;

    ASSERT_EQUAL_D(1, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(4, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");

    v = I2V 999;
    v = mpmc_ring_queue_dequeue(q);
    ASSERT_NULL_D(v, "queue must be empty");
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    /* retry empty  check */
    ASSERT_EQUAL_D(1, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(4, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");

    v = I2V 111;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue (1)");
    ASSERT_EQUAL_U_D((size_t) q->msgs[0].d, (size_t) v,
                     "queue_enqueue (0) not set");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(3, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.put_pos = 1;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = I2V 222;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue (1)");
    ASSERT_EQUAL_U_D((size_t) q->msgs[1].d, (size_t) v,
                     "queue_enqueue (1) not set");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(2, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.put_pos = 2;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = I2V 333;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue (2)");
    ASSERT_EQUAL_U_D((size_t) q->msgs[2].d, (size_t) v,
                     "queue_enqueue (2) not set");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(1, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.put_pos = 3;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = I2V 444;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue (3)");
    ASSERT_EQUAL_U_D((size_t) q->msgs[3].d, (size_t) v,
                     "queue_enqueue (2) not set");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(1, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(0, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.put_pos = 4;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = I2V 555;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_FULL, qerr, "queue_enqueue (full)");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(1, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(0, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");

    v = mpmc_ring_queue_dequeue(q);
    ASSERT_EQUAL_U_D(111, (size_t) v, "queue_dequeue (0)");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(1, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.get_pos = 1;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = mpmc_ring_queue_dequeue(q);
    ASSERT_EQUAL_U_D(222, (size_t) v, "queue_dequeue (1)");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(2, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.get_pos = 2;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = I2V 555;
    qerr = mpmc_ring_queue_enqueue(q, v);
    ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue (0)");
    ASSERT_EQUAL_U_D((size_t) q->msgs[0].d, (size_t) v,
                     "queue_enqueue (0) not set value");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(1, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.put_pos = 5;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = mpmc_ring_queue_dequeue(q);
    ASSERT_EQUAL_U_D(333, (size_t) v, "queue_dequeue (2)");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(2, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.get_pos = 3;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = mpmc_ring_queue_dequeue(q);
    ASSERT_EQUAL_U_D(444, (size_t) v, "queue_dequeue (3)");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(3, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.get_pos = 4;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    v = mpmc_ring_queue_dequeue(q);
    ASSERT_EQUAL_U_D(555, (size_t) v, "queue_dequeue (4)");
    ASSERT_EQUAL_D(1, mpmc_ring_queue_empty(q), "queue_is_empthy");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q), "queue_is_full");
    ASSERT_EQUAL_U_D(4, mpmc_ring_queue_free_relaxed(q), "queue_free_relaxed");
    /* check for memory corruption */
    save_q.get_pos = 5;
    ASSERT_DATA_D((const unsigned char *) &save_q, sizeof(save_q),
                  (const unsigned char *) q, sizeof(save_q),
                  "queue can be corrupt");

    /* delete queue */
    ASSERT_EQUAL_D(QERR_OK, mpmc_ring_queue_destroy(q), "queue_delete");
} /* CTEST(mpmc_ring_queue_base, queue_dequeue) */

static void mpmc_ring_queue_dump(mpmc_ring_queue *q) {
    size_t i;
    printf("\n  put_pos = %lu\n", (unsigned long) q->put_pos);
    printf("  get_pos = %lu\n", (unsigned long) q->get_pos);
    for (i = 0; i < mpmc_ring_queue_size(q); i++) {
        unsigned long m = (unsigned long) q->msgs[i].d;
        if (i == q->msgs[i].seq) {
            printf("  [%lu] = %lu\n", (unsigned long) i, m);
        }
    }
}

/* simulate enqueue/dequeue to (last - 1) element, and enqueue to last element
 * with SIZE_MAX position */
static void mpmc_ring_queue_seek(mpmc_ring_queue *q) {
    size_t i, n;

    n = SIZE_MAX / mpmc_ring_queue_size(q) * mpmc_ring_queue_size(q);
    for (i = 0; i < mpmc_ring_queue_size(q) - 1; i++) {
        q->msgs[i].d = NULL;
        q->msgs[i].seq = n + i + 1 + q->capacity_mod;
    }
    q->msgs[i].d = I2V 1;
    q->msgs[i].seq = n + i + 1;
    q->put_pos = 0;
    // q->get_pos = (size_t) -1;
    q->get_pos = n + q->capacity_mod;
}

typedef struct {
    mpmc_ring_queue *q;
    size_t pos;
} mpmc_ring_queue_pos;

/* complete dequeue */
static void *overflow_dequeue_thread(void *in_arg) {
    mpmc_ring_queue_pos *q_pos = (mpmc_ring_queue_pos *) in_arg;

    usleep(200);

    __atomic_store_n(&q_pos->q->get_pos, q_pos->pos + 1, __ATOMIC_RELAXED);
    q_pos->q->msgs[q_pos->pos & q_pos->q->capacity_mod].d = NULL;

    return NULL;
}

/* check size_t overflow workaround on dequeue */
CTEST(mpmc_ring_queue_size_max, overflow_dequeue) {
    pthread_attr_t thr_attr;
    pthread_t thr_handle;
    mpmc_ring_queue_pos q_pos;
    int perr;
    q_pos.q = mpmc_ring_queue_new(4, NULL);
    ASSERT_NOT_NULL(q_pos.q);

    mpmc_ring_queue_seek(q_pos.q);
    ASSERT_EQUAL_D(0, mpmc_ring_queue_empty(q_pos.q), "queue_is_empty");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q_pos.q), "queue_is_full");
    ASSERT_EQUAL_U(mpmc_ring_queue_size(q_pos.q) - 1, mpmc_ring_queue_free_relaxed(q_pos.q));

    q_pos.pos = q_pos.q->get_pos;
    /* simulate dequeue n other thread after get_pos was read in
     * mpmc_ring_queue_dequeue */
    __atomic_store_n(&q_pos.q->msgs[q_pos.pos & q_pos.q->capacity_mod].seq,
                     q_pos.pos + 1 + q_pos.q->capacity_mod, __ATOMIC_RELEASE);

    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);

    /* complete dequeue in other thread */
    perr =
        pthread_create(&thr_handle, &thr_attr, overflow_dequeue_thread, &q_pos);
    ASSERT_EQUAL_D(0, perr, "overflow_dequeue_thread");

    ASSERT_NULL(mpmc_ring_queue_dequeue(q_pos.q));

    pthread_join(thr_handle, NULL);

    ASSERT_EQUAL_D(1, mpmc_ring_queue_empty(q_pos.q), "queue_is_empty");
    ASSERT_EQUAL_D(0, mpmc_ring_queue_full(q_pos.q), "queue_is_full");
    ASSERT_EQUAL_U(mpmc_ring_queue_size(q_pos.q), mpmc_ring_queue_free_relaxed(q_pos.q));
}

typedef struct {
    char padding1[CACHE_LINE_SIZE - sizeof(int)];
    size_t enq_sum;
    size_t enq_cnt;
    size_t enq_num;
    size_t deq_sum;
    size_t deq_cnt;
    mpmc_ring_queue *q;
    size_t loops;
    int verify; // disable check order (for multithread testing)
    pthread_barrier_t barrier;
} tst_t;

static void *enq_thread(void *in_arg) {
    tst_t *test_data = (tst_t *) in_arg;
    mpmc_ring_queue *q = test_data->q;
    size_t msg_num;
    pthread_barrier_wait(&test_data->barrier);
    msg_num = __sync_add_and_fetch(&test_data->enq_num, 1);
    while (msg_num <= test_data->loops) {
        qerr_t qerr = mpmc_ring_queue_enqueue(q, I2V msg_num);
        if (qerr == QERR_OK) {
            // printf("ENQUEUE %lu AT %lu\n", (unsigned long) msg_num,
            //        (unsigned long) q->put_pos);
            __sync_fetch_and_add(&test_data->enq_sum, msg_num);
            msg_num = __sync_add_and_fetch(&test_data->enq_num, 1);
            __sync_add_and_fetch(&test_data->enq_cnt, 1);
        } else if (qerr == QERR_FULL) {
            // printf("ENQUEUE %lu FULL\n", (unsigned long) msg_num);
            continue;
        } else {
            ASSERT_EQUAL_D(QERR_OK, qerr, "queue_enqueue");
        }
    } /* while */

    pthread_exit(NULL);
    return NULL; /* don't need this, but keep the compiler happy */
} /* enq_thread */

static void *deq_thread(void *in_arg) {
    tst_t *test_data = (tst_t *) in_arg;
    mpmc_ring_queue *q = test_data->q;
    size_t msg_num = 0;

    pthread_barrier_wait(&test_data->barrier);
    while (msg_num < test_data->loops) {
        size_t m = (size_t) mpmc_ring_queue_dequeue(q);
        if (m) {
            unsigned long get_pos = (unsigned long) q->get_pos;
            // printf("DEQUEUE %lu AT %lu\n", (unsigned long) m, get_pos);
            msg_num = __sync_add_and_fetch(&test_data->deq_cnt, 1);
            __sync_fetch_and_add(&test_data->deq_sum, m);
            if (test_data->verify && m != msg_num) {
                fprintf(stderr, "DEQUEUE GOT %lu (WANT %lu) AT %lu\n",
                        (unsigned long) m, (unsigned long) msg_num, get_pos);
                abort();
            }
        } else {
            msg_num = __sync_add_and_fetch(&test_data->deq_cnt, 0);
        }
    } /* while */

    pthread_exit(NULL);
    return NULL; /* don't need this, but keep the compiler happy */
} /* deq_thread */

void mpmc_ring_queue_enqueue_dequeue(size_t q_size, size_t loops, unsigned writers,
                                unsigned readers, int verify) {
    /* Prepare for threading tests */
    qerr_t qerr;
    int perr;
    uint64_t start, end;
    unsigned i;

    pthread_attr_t thr_attr;
    pthread_t *deq_t_handles, *enq_t_handles;

    tst_t test_data; /* shared with test threads */

    ASSERT_NOT_EQUAL_U(q_size, 0);
    ASSERT_NOT_EQUAL_U(loops, 0);
    ASSERT_NOT_EQUAL_U(writers, 0);
    ASSERT_NOT_EQUAL_U(readers, 0);

    deq_t_handles = malloc(sizeof(pthread_t) * readers);
    ASSERT_NOT_NULL(deq_t_handles);
    enq_t_handles = malloc(sizeof(pthread_t) * writers);
    ASSERT_NOT_NULL(enq_t_handles);

    test_data.q = mpmc_ring_queue_new(q_size, &qerr);
    ASSERT_NOT_NULL(test_data.q);

    /* one element with 1 value in queue */
    mpmc_ring_queue_seek(test_data.q);

    test_data.loops = loops;
    test_data.enq_num = 1;
    test_data.enq_cnt = 1;
    test_data.enq_sum = 1;
    test_data.deq_cnt = 0;
    test_data.deq_sum = 0;
    test_data.verify = verify;

    ASSERT_EQUAL_D(
        0,
        pthread_barrier_init(&test_data.barrier, NULL, writers + readers + 1),
        "pthread_barrier_init");

    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < readers; i++) {
        perr = pthread_create(&deq_t_handles[i], &thr_attr, deq_thread,
                              &test_data);
        ASSERT_EQUAL_D(0, perr, "deq_thread create");
    }

    for (i = 0; i < writers; i++) {
        perr = pthread_create(&enq_t_handles[i], &thr_attr, enq_thread,
                              &test_data);
        ASSERT_EQUAL_D(0, perr, "enq_thread create");
    }

    pthread_attr_destroy(&thr_attr);

    pthread_barrier_wait(&test_data.barrier);
    start = getCurrentTime();

    /* wait for threads to complete. */
    for (i = 0; i < writers; i++) {
        perr = pthread_join(enq_t_handles[i], NULL);
        ASSERT_EQUAL_D(0, perr, "enq_thread join");
    }
    for (i = 0; i < readers; i++) {
        perr = pthread_join(deq_t_handles[i], NULL);
        ASSERT_EQUAL_D(0, perr, "deq_thread join");
    }

    end = getCurrentTime();

    pthread_barrier_destroy(&test_data.barrier);

    if (test_data.deq_cnt != test_data.enq_cnt ||
        test_data.deq_sum != test_data.enq_sum) {
        mpmc_ring_queue_dump(test_data.q);
        mpmc_ring_queue_destroy(test_data.q);
        ASSERT_FORMAT(
            "enqueue/dequeue cnt = %lu/%lu, enqueue/dequeue sum = %lu/%lu",
            (unsigned long) test_data.enq_cnt,
            (unsigned long) test_data.deq_cnt,
            (unsigned long) test_data.enq_sum,
            (unsigned long) test_data.deq_sum);
    }

    ASSERT_EQUAL_D(QERR_OK, mpmc_ring_queue_destroy(test_data.q), "queue_destroy");

    printf("(%f ms, %lu iterations, %llu ns/op) ",
           ((double) end - (double) start) / 1000,
           (unsigned long) test_data.loops,
           (unsigned long long) (end - start) * 1000 / test_data.loops);
}

CTEST(mpmc_ring_queue_thread, enqueue_dequeue) {
    mpmc_ring_queue_enqueue_dequeue(4096, 10000000, 1, 1, 0);
}

CTEST(mpmc_ring_queue_thread, enqueue4_dequeue4) {
    mpmc_ring_queue_enqueue_dequeue(32, 10000000, 4, 4, 0);
}

CTEST(mpmc_ring_queue_thread, enqueue6_dequeue4) {
    mpmc_ring_queue_enqueue_dequeue(32, 10000000, 6, 6, 0);
}

CTEST(mpmc_ring_queue_thread, enqueue4_dequeue6) {
    mpmc_ring_queue_enqueue_dequeue(32, 10000000, 4, 6, 0);
}

CTEST(mpmc_ring_queue_thread, enqueue6_dequeue6) {
    mpmc_ring_queue_enqueue_dequeue(32, 10000000, 6, 6, 0);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
