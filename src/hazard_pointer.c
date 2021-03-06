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

#include <fibconcurrent/hazard_pointer.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>

hazard_pointer_thread_record_t* hazard_pointer_thread_record_create_and_push(hazard_pointer_thread_record_t** head, size_t pointers_per_thread)
{
    size_t sizeof_pointers, required_size;
    hazard_pointer_thread_record_t *ret, *cur_head, *cur;

    assert(head);
    assert(pointers_per_thread);

    //create a new record
    sizeof_pointers = pointers_per_thread * sizeof(*((*head)->hazard_pointers));
    required_size = sizeof(hazard_pointer_thread_record_t) + sizeof_pointers;
    ret = (hazard_pointer_thread_record_t*)calloc(1, required_size);
    ret->head = head;
    ret->hazard_pointers_count = pointers_per_thread;
    write_barrier();//finish all writes before exposing the record to the other threads

    //swap in the new record as the head
    do {
        size_t threads = 1;//1 for this thread

        cur_head = *head;
        ret->next = cur_head;

        //determine the appropriate retire_threshold. head should always have the correct retire_threshold, so this must be done before swapping ret in as head
        cur = ret->next;
        while(cur) {
            ++threads;
            assert(cur->hazard_pointers_count == ret->hazard_pointers_count);
            cur = cur->next;
        }
        ret->retire_threshold = 2 * threads * pointers_per_thread;
    } while(!__sync_bool_compare_and_swap(head, cur_head, ret));

    //update all other threads' retire thresholds
    cur = ret->next;
    while(cur) {
        __sync_add_and_fetch(&cur->retire_threshold, 2 * cur->hazard_pointers_count);//we're increasing N by 1, so R increases by 2 * K (remember R = 2 * N * K)
        cur = cur->next;
    }

    return ret;
}

void hazard_pointer_thread_record_destroy_all(hazard_pointer_thread_record_t* head)
{
    hazard_pointer_thread_record_t* cur = head;
    while(cur) {
        hazard_pointer_thread_record_t* next;
        cur->head = &cur;
        next = cur->next;
        hazard_pointer_thread_record_destroy(cur);
        cur = next;
    }
}

void hazard_pointer_thread_record_destroy(hazard_pointer_thread_record_t* hptr)
{
    if(hptr) {
        hazard_pointer_scan(hptr);//attempt to cleanup; best effort only here. really no threads should still be using these hazard pointers, so all should be freed
        free(hptr->plist);
    }
    free(hptr);
}

static inline int hazard_pointer_compare(const void* p_one, const void* p_two)
{
    const uintptr_t one = *(uintptr_t*)p_one;
    const uintptr_t two = *(uintptr_t*)p_two;
    if(one == two) {
        return 0;
    }
    return one < two ? -1 : 1;
}

static int binary_search(void** haystack, ssize_t haystack_size, void* needle)
{
    ssize_t start;
    ssize_t end;

    assert(haystack);
    if(!haystack_size) {
        return 0;
    }
    start = 0;
    end = haystack_size - 1;
    while(start <= end) {
        ssize_t middle;
        void *middle_value;
        assert(start >= 0 && start < haystack_size);
        assert(end >= 0 && end < haystack_size);
        middle = (start + end) / 2;
        assert(middle >= 0 && middle < haystack_size);
        middle_value = haystack[middle];
        if(middle_value > needle) {
            end = middle - 1;
        } else if(middle_value < needle) {
            start = middle + 1;
        } else {
            return 1;
        }
    }
    return 0;
}

void hazard_pointer_scan(hazard_pointer_thread_record_t* hptr)
{
    hazard_pointer_thread_record_t *head, *cur_record;
    hazard_node_t* node;
    size_t index, i, max_pointers;

    assert(hptr);
    //head always has a correct retired_threshold; that is, retired_threshold = 2 * N * K
    head = *hptr->head;
    assert(head);
    max_pointers = head->retire_threshold / 2;
    if(!hptr->plist || hptr->plist_size < max_pointers) {
        free(hptr->plist);
        hptr->plist_size = max_pointers;
        hptr->plist = (hazard_node_t**)malloc(max_pointers * sizeof(*hptr->plist));
    }

    index = 0;
    cur_record = head;
    while(cur_record) {
        size_t hazard_pointers_count;
        hazard_node_t **hazard_pointers;
        assert(index < max_pointers);
        hazard_pointers_count = cur_record->hazard_pointers_count;
        hazard_pointers = &*cur_record->hazard_pointers;
        for(i = 0; i < hazard_pointers_count; ++i) {
            hazard_node_t* const h = hazard_pointers[i];
            if(h) {
                hptr->plist[index] = h;
                ++index;
            }
        }
        cur_record = cur_record->next;
    }

    qsort(hptr->plist, index, sizeof(*hptr->plist), &hazard_pointer_compare);

    node = hptr->retired_list;
    hptr->retired_list = NULL;
    hptr->retired_count = 0;

    while(node) {
        hazard_node_t* const next = node->next;

        const int is_hazardous = binary_search((void**)hptr->plist, (ssize_t) index, node);

        if(is_hazardous) {
            node->next = hptr->retired_list;
            hptr->retired_list = node;
            ++hptr->retired_count;
        } else {
            assert(node->gc_function);
            node->gc_function(node->gc_data, node);
        }
        node = next;
    }
}

