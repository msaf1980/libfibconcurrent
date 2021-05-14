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

#include <concurrent/queue.h>

#include <concurrent/arch.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

#define V2I (size_t)(void *)
#define I2V (void *) (size_t)

typedef struct {
    char padding1[CACHE_LINE_SIZE - sizeof(int)];
    size_t enq_sum;
    size_t enq_cnt;
    size_t enq_num;
    size_t deq_sum;
    size_t deq_cnt;
    queue *q;
    size_t loops;
    int verify; /* disable check order (for multithread testing) */
    pthread_barrier_t barrier;
} tst_t;

static void *enq_thread(void *in_arg) {
    tst_t *test_data = (tst_t *) in_arg;
    queue *q = test_data->q;
    size_t msg_num;
    pthread_barrier_wait(&test_data->barrier);
    msg_num = __sync_add_and_fetch(&test_data->enq_num, 1);
    while (msg_num <= test_data->loops) {
        if (queue_enqueue(q, (const char*) msg_num)) {
            __sync_fetch_and_add(&test_data->enq_sum, msg_num);
            msg_num = __sync_add_and_fetch(&test_data->enq_num, 1);
            __sync_add_and_fetch(&test_data->enq_cnt, 1);
        } else {
            continue;
        }
    } /* while */

    pthread_exit(NULL);
    return NULL; /* don't need this, but keep the compiler happy */
} /* enq_thread */

static void *deq_thread(void *in_arg) {
    tst_t *test_data = (tst_t *) in_arg;
    queue *q = test_data->q;
    size_t msg_num = 0;

    pthread_barrier_wait(&test_data->barrier);
    while (msg_num < test_data->loops) {
        size_t m = (size_t) queue_dequeue(q);
        if (m) {
            msg_num = __sync_add_and_fetch(&test_data->deq_cnt, 1);
            __sync_fetch_and_add(&test_data->deq_sum, m);
            if (test_data->verify && m != msg_num) {
                fprintf(stderr, "DEQUEUE GOT %lu (WANT %lu)\n",
                        (unsigned long) m, (unsigned long) msg_num);
                abort();
            }
        } else {
            msg_num = __sync_add_and_fetch(&test_data->deq_cnt, 0);
        }
    } /* while */

    pthread_exit(NULL);
    return NULL; /* don't need this, but keep the compiler happy */
} /* deq_thread */

void queue_enqueue_dequeue(size_t q_size, size_t loops, unsigned writers,
                                unsigned readers, int verify) {
    /* Prepare for threading tests */
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

    test_data.q = queue_new(q_size);
    ASSERT_NOT_NULL(test_data.q);

    test_data.loops = loops;
    test_data.enq_num = 0;
    test_data.enq_cnt = 0;
    test_data.enq_sum = 0;
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
        queue_destroy(test_data.q);
        ASSERT_FORMAT(
            "enqueue/dequeue cnt = %lu/%lu, enqueue/dequeue sum = %lu/%lu",
            (unsigned long) test_data.enq_cnt,
            (unsigned long) test_data.deq_cnt,
            (unsigned long) test_data.enq_sum,
            (unsigned long) test_data.deq_sum);
    }

    queue_destroy(test_data.q);

    printf("(%f ms, %lu iterations, %llu ns/op) ",
           ((double) end - (double) start) / 1000,
           (unsigned long) test_data.loops,
           (unsigned long long) (end - start) * 1000 / test_data.loops);
}

CTEST(queue_thread, enqueue_dequeue) {
    queue_enqueue_dequeue(4096, 10000000, 1, 1, 0);
}

CTEST(queue_thread, enqueue4_dequeue4) {
    queue_enqueue_dequeue(32, 10000000, 4, 4, 0);
}

CTEST(queue_thread, enqueue6_dequeue4) {
    queue_enqueue_dequeue(32, 10000000, 6, 6, 0);
}

CTEST(queue_thread, enqueue4_dequeue6) {
    queue_enqueue_dequeue(32, 10000000, 4, 6, 0);
}

CTEST(queue_thread, enqueue6_dequeue6) {
    queue_enqueue_dequeue(32, 10000000, 6, 6, 0);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
