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

#include <concurrent/hazard_pointer.h>
#include <concurrent/lockfree_ring_buffer.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

//NOTE: to be run with valgrind to detect memory leaks or invalid memory access

#define PER_THREAD_COUNT 10000
#define NUM_THREADS 4
#define POINTERS_PER_THREAD 4

hazard_pointer_thread_record_t* head = NULL;

struct test_object
{
    hazard_node_t hazard_node;
    int my_data;
};

pthread_barrier_t barrier;
lockfree_ring_buffer_t* free_nodes = 0;

void release_node(void* user_data, hazard_node_t* node)
{
    (void) user_data;
    lockfree_ring_buffer_push(free_nodes, node);
}

hazard_node_t* get_node(hazard_pointer_thread_record_t* hptr)
{
    hazard_node_t* node;
    while(!(node = lockfree_ring_buffer_trypop(free_nodes))) {
        hazard_pointer_scan(hptr);
    }
    node->gc_data = NULL;
    node->gc_function = &release_node;
    return node;
}

hazard_pointer_thread_record_t* records[NUM_THREADS];

void* run_function(void* param)
{
    hazard_pointer_thread_record_t* my_record = hazard_pointer_thread_record_create_and_push(&head, POINTERS_PER_THREAD);
    hazard_node_t* nodes[POINTERS_PER_THREAD];
    size_t i;
    pthread_barrier_wait(&barrier);
    records[(intptr_t)param] = my_record;
    for(i = 0; i < PER_THREAD_COUNT; ++i) {
        size_t j;
        for(j = 0; j < POINTERS_PER_THREAD; ++j) {
            nodes[j] = get_node(my_record);
            hazard_pointer_using(my_record, nodes[j], j);
        }

        for(j = 0; j < POINTERS_PER_THREAD; ++j) {
            hazard_pointer_done_using(my_record, j);
            hazard_pointer_free(my_record, nodes[j]);
        }
    }
    //need to finish cleaning records owned by this thread. M.M. Michael's paper suggests methods to let threads leave...not bothering
    while(my_record->retired_list) {
        hazard_pointer_scan(my_record);
    }
    return NULL;
}

CTEST(hazard_pointer, threaded)
{
    const size_t BUFFER_SIZE = NUM_THREADS * 2 * NUM_THREADS * POINTERS_PER_THREAD; //each thread can have up to 2 * N * K, so we need up to N * 2 * N * K nodes available at any given time
    size_t node_count;
    pthread_t threads[NUM_THREADS];
    intptr_t i;
    size_t count = 0;    
    hazard_pointer_thread_record_t* cur;
    void* to_free;

    ASSERT_TRUE(BUFFER_SIZE <= (1 << 10));
    free_nodes = lockfree_ring_buffer_create(10);    
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    for(node_count = 0; node_count < BUFFER_SIZE; ++node_count) {
        lockfree_ring_buffer_push(free_nodes, malloc(sizeof(hazard_node_t)));
    }

    for(i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, &run_function, (void*)i);
    }

    for(i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    ASSERT_NOT_NULL(head);
    cur = head;
    while(cur) {
        ASSERT_EQUAL_U(POINTERS_PER_THREAD, cur->hazard_pointers_count);
        ASSERT_EQUAL_U(2 * NUM_THREADS * POINTERS_PER_THREAD, cur->retire_threshold);
        hazard_pointer_scan(cur);

        cur = cur->next;
        ++count;
    }
    ASSERT_EQUAL_U(NUM_THREADS, count);

    hazard_pointer_thread_record_destroy_all(head);
    while((to_free = lockfree_ring_buffer_trypop(free_nodes))) {
        free(to_free);
    }
    lockfree_ring_buffer_destroy(free_nodes);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
