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

#include <fibconcurrent/mpmc_stack.h>

#include <stdint.h>
#include <unistd.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

#define NUM_THREADS 4
#define PER_THREAD_COUNT 2500000

int pushes[NUM_THREADS];
int pops[NUM_THREADS];
int volatile done[NUM_THREADS];
pthread_barrier_t barrier;

mpmc_stack_t the_q;

void* push_func(void* p)
{
    intptr_t thr = (intptr_t)p;
    intptr_t i = 0;
    pthread_barrier_wait(&barrier);    
    while(!done[thr]) {
        mpmc_stack_node_t* n = malloc(sizeof(mpmc_stack_node_t));
        ++i;        
        mpmc_stack_node_init(n, (void*)i);
        while(MPMC_RETRY == mpmc_stack_push_timeout(&the_q, n, 10)) {
            sched_yield();
        }
    }
    return NULL;
}

void* pop_func(void* p)
{
    intptr_t thr = (intptr_t)p;
    intptr_t last = 0;
    intptr_t counter = 0;    
    (void)last;
    pthread_barrier_wait(&barrier);    
    while(!done[thr]) {
        mpmc_stack_node_t* head = NULL;
        while(MPMC_RETRY == mpmc_stack_fifo_flush_timeout(&the_q, &head, 10)) {
            sched_yield();
        }
        if(!head) {
            usleep(1);
        }
        while(head) {
            mpmc_stack_node_t* old;
            intptr_t i = (intptr_t)mpmc_stack_node_get_data(head);
            if(NUM_THREADS == 1) {
                assert(i > last);
                last = i;
            }
            ++counter;
            if(counter > PER_THREAD_COUNT) {
                done[thr] = 1;
            }
            old = head;
            head = head->next;
            free(old);
        }
    }
    return NULL;
}

CTEST(mpmc_stack, threaded)
{
    int i;
    pthread_t reader[NUM_THREADS];
    pthread_t writer[NUM_THREADS];

    pthread_barrier_init(&barrier, NULL, 2 * NUM_THREADS);
    mpmc_stack_init(&the_q);

    for(i = 0; i < NUM_THREADS; ++i) {
        pushes[i] = 0;
        pops[i] = 0;
        done[i] = 0;
    }
    for(i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&reader[i], NULL, &pop_func, (void*)(intptr_t)i);
    }
    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_create(&writer[i], NULL, &push_func, (void*)(intptr_t)i);
    }

    push_func(0);

    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_join(reader[i], NULL);
    }
    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_join(writer[i], NULL);
    }
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
