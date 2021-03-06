/*
 * Copyright (c) 2012-2015, Brian Watling and other contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <fibconcurrent/mpsc_fifo.h>
#include <stdint.h>
#include <unistd.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

#define PUSH_COUNT 10000000
#define NUM_THREADS 4

mpsc_fifo_t fifo;
int results[PUSH_COUNT];
pthread_barrier_t barrier;

void* push_func(void* p)
{
    intptr_t i;

    (void) p;
    pthread_barrier_wait(&barrier);
    for(i = 0; i < PUSH_COUNT; ++i) {
        mpsc_fifo_node_t* const node = malloc(sizeof(mpsc_fifo_node_t));
        node->data = (void*)i;
        mpsc_fifo_push(&fifo, node);
    }
    return NULL;
}

CTEST(mpsc_fifo, threaded)
{
    pthread_t producers[NUM_THREADS];
    intptr_t i = 0;
    mpsc_fifo_node_t* node = NULL;

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    mpsc_fifo_init(&fifo);

    for(i = 0; i < PUSH_COUNT; ++i) {
        results[i] = 0;
    }

    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_create(&producers[i], NULL, &push_func, NULL);
    }

    pthread_barrier_wait(&barrier);

    for(i = 0; i < PUSH_COUNT * (NUM_THREADS-1); ++i) {
        void* data = NULL;
        while(!mpsc_fifo_peek(&fifo, &data)) {};
        node = mpsc_fifo_trypop(&fifo);
        ASSERT_NOT_NULL(node);
        ASSERT_TRUE(node->data == data);
        ++results[(intptr_t)node->data];
        free(node);
    }

    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_join(producers[i], 0);
    }

    for(i = 0; i < PUSH_COUNT; ++i) {
        ASSERT_EQUAL(NUM_THREADS - 1, results[i]);
    }

    printf("cleaning...\n");
    mpsc_fifo_destroy(&fifo);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
