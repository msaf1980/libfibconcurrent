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

#include <fibconcurrent/work_stealing_deque.h>

#include <pthread.h>
#if !HAVE_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

#define SHARED_COUNT 5000000
wsd_work_stealing_deque_t* wsd_d2 = NULL;
#define NUM_THREADS 4
int results[NUM_THREADS][SHARED_COUNT];
size_t run_func_count[NUM_THREADS];
int64_t total = 0;
int done = 0;

void* run_func(void* p)
{
    intptr_t threadId = (intptr_t)p;
    while(!done) {
        void* ret = wsd_work_stealing_deque_steal(wsd_d2);
        if(ret != WSD_EMPTY && ret != WSD_ABORT) {
            __sync_add_and_fetch(&results[threadId][(intptr_t)ret], 1);
            __sync_add_and_fetch(&total, (intptr_t)ret);
            ++run_func_count[threadId];
        }
    }
    return NULL;
}

CTEST(wsd_circular_array, single)
{
    wsd_circular_array_t *wsd_a;

    wsd_a = wsd_circular_array_create(8);
    ASSERT_NOT_NULL(wsd_a);
    ASSERT_EQUAL_U(256, wsd_circular_array_size(wsd_a));
    wsd_circular_array_put(wsd_a, 1, (void*)1);
    ASSERT_EQUAL(1, (intptr_t) wsd_circular_array_get(wsd_a, 1));
    wsd_circular_array_destroy(wsd_a);
}

CTEST(wsd_work_stealing_deque_t, single)
{
    int i;
     wsd_circular_array_t* old;

    wsd_work_stealing_deque_t *wsd_d = wsd_work_stealing_deque_create(0);
    ASSERT_NOT_NULL(wsd_d);
    for(i = 0; i < 1000; ++i) {
       wsd_work_stealing_deque_push_bottom_autogrow(wsd_d, (void*)(intptr_t)i, &old);
    }
    for(i = 1000; i > 0; --i) {
        void* item = wsd_work_stealing_deque_pop_bottom(wsd_d);
        ASSERT_EQUAL(i - 1, (intptr_t)item);
    }
    wsd_work_stealing_deque_destroy(wsd_d);
}

CTEST(wsd_circular_array, threaded)
{
    int i, j;
    void* val;
    int64_t expected_total = 0;    
    pthread_t reader[NUM_THREADS];
    wsd_circular_array_t* old;

    for(i = 0; i < NUM_THREADS; ++i) {
        run_func_count[i] = 0;
        for(j = 0; j < SHARED_COUNT; ++j) {
            results[i][j] = 0;
        }
    }


    wsd_d2 = wsd_work_stealing_deque_create(0);
    ASSERT_NOT_NULL(wsd_d2);
    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_create(&reader[i], NULL, &run_func, (void*)(intptr_t)i);
    }

    for(i = 0; i < SHARED_COUNT; ++i) {
        wsd_work_stealing_deque_push_bottom_autogrow(wsd_d2, (void*)(intptr_t)i, &old);
        if((i & 7) == 0) {
            val = wsd_work_stealing_deque_pop_bottom(wsd_d2);
            if(val != WSD_EMPTY && val != WSD_ABORT) {
                __sync_add_and_fetch(&results[0][(intptr_t)val], 1);
                __sync_add_and_fetch(&total, (intptr_t)val);
                ++run_func_count[0];
            }
        }
    }

    val = 0;
    do {
        val = wsd_work_stealing_deque_pop_bottom(wsd_d2);
        if(val != WSD_EMPTY && val != WSD_ABORT) {
            __sync_add_and_fetch(&results[0][(intptr_t)val], 1);
            __sync_add_and_fetch(&total, (intptr_t)val);
            ++run_func_count[0];
        }
    } while(val != WSD_EMPTY);

    done = 1;
    for(i = 1; i < NUM_THREADS; ++i) {
        pthread_join(reader[i], NULL);
    }

    for(i = 0; i < SHARED_COUNT; ++i) {
        int sum = 0;
        for(j = 0; j < NUM_THREADS; ++j) {
            sum += results[j][i];
        }
        ASSERT_EQUAL(1, sum);
        expected_total += i;
    }
    ASSERT_EQUAL(total, expected_total);
    for(i = 0; i < NUM_THREADS; ++i) {
        ASSERT_TRUE(run_func_count[i] > 0);
    }
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */
