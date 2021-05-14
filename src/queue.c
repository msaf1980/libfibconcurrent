/*
 * Copyright 2013-2018 Fabian Groffen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <concurrent/queue.h>

struct _queue {
	const char **queue;
	size_t end;
	size_t write;
	size_t read;
	size_t len;
	pthread_mutex_t lock;
};


queue *
queue_new(size_t size)
{
	queue *ret = malloc(sizeof(queue));

	if (ret == NULL)
		return NULL;

	ret->queue = malloc(sizeof(char *) * size);
	if (ret->queue == NULL) {
		free(ret);
		return NULL;
	}

	memset(ret->queue, 0, sizeof(char *) * size);
	ret->end = size;
	ret->read = ret->write = 0;
	ret->len = 0;
	pthread_mutex_init(&ret->lock, NULL);

	return ret;
}

void
queue_destroy(queue *q)
{
	pthread_mutex_destroy(&q->lock);
	free(q->queue);
	free(q);
}


void queue_delete(queue *q, free_func f)
{
	const char *p;
    if (f == NULL) {
        f = free;
    }
    while ((p = queue_dequeue(q)) != NULL) {
        f((void *) p);
    }

    queue_destroy(q);
}

int
queue_enqueue(queue *q, const char *p)
{
	/* queue normal:
	 * |=====-----------------------------|  4
	 * ^    ^
	 * r    w
	 * queue wrap:
	 * |===---------------------------====|  6
	 *    ^                           ^
	 *    w                           r
	 * queue full
	 * |==================================| 23
	 *          ^
	 *         w+r
	 */
	pthread_mutex_lock(&q->lock);
	if (q->len == q->end) {
		pthread_mutex_unlock(&q->lock);
		return 0;
	}
	if (q->write == q->end)
		q->write = 0;
	q->queue[q->write] = p;
	q->write++;
	q->len++;
	pthread_mutex_unlock(&q->lock);
	return 1;
}

const char *
queue_dequeue(queue *q)
{
	const char *ret;
	pthread_mutex_lock(&q->lock);
	if (q->len == 0) {
		pthread_mutex_unlock(&q->lock);
		return NULL;
	}
	if (q->read == q->end)
		q->read = 0;
	ret = q->queue[q->read++];
	q->len--;
	pthread_mutex_unlock(&q->lock);
	return ret;
}

size_t
queue_dequeue_vector(const char **ret, queue *q, size_t len)
{
	size_t i;

	pthread_mutex_lock(&q->lock);
	if (q->len == 0) {
		pthread_mutex_unlock(&q->lock);
		return 0;
	}
	if (len > q->len)
		len = q->len;
	for (i = 0; i < len; i++) {
		if (q->read == q->end)
			q->read = 0;
		ret[i] = q->queue[q->read++];
	}
	q->len -= len;
	pthread_mutex_unlock(&q->lock);

	return len;
}

char
queue_putback(queue *q, const char *p)
{
	pthread_mutex_lock(&q->lock);
	if (q->len == q->end) {
		pthread_mutex_unlock(&q->lock);
		return 0;
	}

	if (q->read == 0)
		q->read = q->end;
	q->read--;
	q->queue[q->read] = p;
	q->len++;
	pthread_mutex_unlock(&q->lock);

	return 1;
}

inline size_t
queue_len(queue *q)
{
	size_t len;
	pthread_mutex_lock(&q->lock);
	len = q->len;
	pthread_mutex_unlock(&q->lock);
	return len;
}

inline size_t
queue_free(queue *q)
{
	return q->end - queue_len(q);
}

inline size_t
queue_size(queue *q)
{
	return q->end;
}
