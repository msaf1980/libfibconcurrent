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

#ifndef _MPSC_FIFO_H_
#define _MPSC_FIFO_H_

/*
    Author: Brian Watling
    Email: brianwatling@hotmail.com
    Website: https://github.com/brianwatling
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <concurrent/arch.h>

typedef struct mpsc_fifo_node
{
    void* data;
    struct mpsc_fifo_node* volatile next;
} mpsc_fifo_node_t;

typedef struct mpsc_fifo
{
    mpsc_fifo_node_t* volatile head;//consumer read items from head
    char _cache_padding1[CACHE_LINE_SIZE - sizeof(mpsc_fifo_node_t*)];
    mpsc_fifo_node_t* tail;//producer pushes onto the tail
} mpsc_fifo_t;

static inline int mpsc_fifo_init(mpsc_fifo_t* f)
{
    assert(f);
    f->tail = (mpsc_fifo_node_t*)calloc(1, sizeof(*f->tail));
    f->head = f->tail;
    if(!f->tail) {
        return 0;
    }
    return 1;
}

static inline void mpsc_fifo_destroy(mpsc_fifo_t* f)
{
    if(f) {
        while(f->head != NULL) {
            mpsc_fifo_node_t* const tmp = f->head;
            f->head = tmp->next;
            free(tmp);
        }
    }
}

//the FIFO owns new_node after pushing
static inline void mpsc_fifo_push(mpsc_fifo_t* f, mpsc_fifo_node_t* new_node)
{
    mpsc_fifo_node_t* prev_tail;
    assert(f);
    assert(new_node);
    new_node->next = NULL;
    prev_tail = (mpsc_fifo_node_t*) __atomic_exchange_n((void**)&f->tail, new_node, __ATOMIC_ACQUIRE);
    prev_tail->next = new_node;
}

//returns 1 if a node is available, 0 otherwise
static inline int mpsc_fifo_peek(mpsc_fifo_t* f, void** data)
{
    mpsc_fifo_node_t* head;
    mpsc_fifo_node_t* next;
    assert(f);
    head = f->head;
    next = head->next;
    if(next) {
        if(data) {
            *data = next->data;
        }
        return 1;
    }
    return 0;
}

//the caller owns the node after popping
static inline mpsc_fifo_node_t* mpsc_fifo_trypop(mpsc_fifo_t* f)
{
    mpsc_fifo_node_t* prev_head;
    mpsc_fifo_node_t* prev_next;
    assert(f);
    prev_head = f->head;
    prev_next = prev_head->next;
    if(prev_next) {
        f->head = prev_next;
        prev_head->data = prev_next->data;
        return prev_head;
    }
    return NULL;
}

#endif
