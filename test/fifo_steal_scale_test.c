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

#include <concurrent/fifo_steal_buffer.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#include <stdlib.h>
#include <sys/time.h>

size_t NUM_THREADS = 4;
size_t PER_THREAD_COUNT = 100000000;
int WORK_FACTOR = 0;
pthread_barrier_t barrier;

long long getusecs(struct timeval* tv)
{
    return (long long)tv->tv_sec * 1000000 + tv->tv_usec;
}

intptr_t do_some_work(intptr_t start)
{
    intptr_t i;
    intptr_t result = start;
    for(i = 0; i < WORK_FACTOR; ++i) {
        result += i + (result & 7);
    }
    return result;
}

typedef struct thread_data
{
    long long push_count;
    long long pop_count;
    long long steal_count;
    long long empty_count;
    long long attempt_count;
    volatile intptr_t dummy;
} __attribute__((__aligned__(CACHE_LINE_SIZE))) thread_data_t;

fifo_steal_buffer_t** fifo = NULL;
thread_data_t* data = NULL;

typedef struct node
{
    struct node* next;
    void* data;
} node_t;

void* run_function(void* param)
{
    intptr_t thread_id = (intptr_t)param;
    unsigned int seed = (unsigned int) thread_id;
    node_t* local_nodes = NULL;    
    fifo_steal_buffer_t* my_fifo = fifo[thread_id];
    thread_data_t* my_data = &(data[thread_id]);
    size_t i;
    pthread_barrier_wait(&barrier);    
    for(i = 0; i < PER_THREAD_COUNT; ++i) {
        const int action = rand_r(&seed) % 10;
//printf("action: %d\n", action);
        if(action < 4) {
            node_t* n = NULL;
            if(fifo_steal_buffer_pop(my_fifo, (void**)&n)) {
                n->next = local_nodes;
                local_nodes = n;
                ++my_data->pop_count;
                my_data->dummy = do_some_work((intptr_t) i);
            }
        } else if(action < 8) {
            node_t* n = NULL;
            if(local_nodes) {
                n = local_nodes;
                local_nodes = local_nodes->next;
            } else {
                n = calloc(1, sizeof(*n));
            }
            n->data = (void*)i;
            assert(fifo_steal_buffer_push(my_fifo, n));
            ++my_data->push_count;
        } else {
            intptr_t j = thread_id + 1;
            intptr_t tries = (intptr_t) NUM_THREADS - 1;//don't steal from yourself
            int stole = 0;
            while(tries > 0) {
                fifo_steal_buffer_t* const steal_fifo = fifo[(size_t) j % NUM_THREADS];
                node_t* n = NULL;
                int ret;
                do {
                    ret = fifo_steal_buffer_steal(steal_fifo, (void**)&n);
                    ++my_data->attempt_count;
                } while(ret == FIFO_STEAL_BUFFER_ABORT);
                if(n) {
                    n->next = local_nodes;
                    local_nodes = n;
                    stole = 1;
                    ++my_data->steal_count;
                    my_data->dummy = do_some_work((intptr_t) i);
                    break;//stole one
                }
                --tries;
                ++j;
                if(!stole) {
                    ++my_data->empty_count;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    size_t i;
    pthread_t threads[NUM_THREADS];
    struct timeval begin, end;
    double us;
    thread_data_t total = { 0, 0, 0, 0, 0, 0 };

    if(argc > 1) {
        NUM_THREADS = (size_t) atoi(argv[1]);
    }
    if(argc > 2) {
        PER_THREAD_COUNT = (size_t) atoi(argv[2]);
    }
    if(argc > 3) {
        WORK_FACTOR = atoi(argv[3]);
    }
    fifo = calloc(NUM_THREADS, sizeof(*fifo));
    data = calloc(NUM_THREADS, sizeof(*data));
    pthread_barrier_init(&barrier, NULL, (unsigned int) NUM_THREADS);

    for(i = 0; i < NUM_THREADS; ++i) {
        fifo[i] = fifo_steal_buffer_create(20);
    }

    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_create(&(threads[i]), NULL, &run_function, (void*)i);
    }

    gettimeofday(&begin, NULL);
    run_function(NULL);

    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);

    for(i = 0; i < NUM_THREADS; ++i) {
        fifo_steal_buffer_destroy(fifo[i]);
        printf("thread %d - push: %lld pop: %lld steal: %lld attempt: %lld empty: %lld\n", (int)i, data[i].push_count, data[i].pop_count, data[i].steal_count, data[i].attempt_count, data[i].empty_count);
        total.push_count += data[i].push_count;
        total.pop_count += data[i].pop_count;
        total.steal_count += data[i].steal_count;
        total.empty_count += data[i].empty_count;
        total.attempt_count += data[i].attempt_count;
    }
    printf("\ntotal - push: %lld pop: %lld steal: %lld attempt: %lld empty: %lld\n", total.push_count, total.pop_count, total.steal_count, total.attempt_count, total.empty_count);

    us = (double) (getusecs(&end) - getusecs(&begin));
    printf("timing: %lu threads %lu events %d work %lf seconds (%.2f ns per item)\n",
        NUM_THREADS, PER_THREAD_COUNT, WORK_FACTOR, us / 1000000.0,
        us * 1000.0 / (double) (total.push_count + total.pop_count + total.empty_count + total.attempt_count)
    );

    return 0;
}

