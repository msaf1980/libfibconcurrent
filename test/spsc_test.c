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

#include <fibconcurrent/spsc_fifo.h>

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

pthread_barrier_t barrier;
spsc_fifo_t fifo;

void* pop_func(void* p)
{
    intptr_t i;
    spsc_node_t* node = NULL;
    (void) p;
    pthread_barrier_wait(&barrier);
    for(i = 0; i < PUSH_COUNT; ++i) {
        while(!(node = spsc_fifo_trypop(&fifo))) {};
        ASSERT_EQUAL(i, (intptr_t)node->data);
        free(node);
    }
    return NULL;
}

CTEST(spsc_fifo, simple)
{
    pthread_t consumer;
    intptr_t i;

    pthread_barrier_init(&barrier, NULL, 2);
    ASSERT_TRUE(spsc_fifo_init(&fifo));

    pthread_create(&consumer, NULL, &pop_func, NULL);

    pthread_barrier_wait(&barrier);

    for(i = 0; i < PUSH_COUNT; ++i) {
        spsc_node_t* const node = malloc(sizeof(spsc_node_t));
        node->data = (void*)i;
        spsc_fifo_push(&fifo, node);
    }

    pthread_join(consumer, NULL);

    printf("cleaning...\n");
    spsc_fifo_destroy(&fifo);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
