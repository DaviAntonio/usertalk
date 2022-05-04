/*
 *  usertalk - a chat room application
 *  Copyright (C) 2022 Davi Antônio da Silva Santos <antoniossdavi at gmail.com>
 *  This file is part of usertalk.

 *  Usertalk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GENERIC_QUEUE_H
#define GENERIC_QUEUE_H

#define DECLARE_CIRCULAR_QUEUE(name, type) \
struct name##_cell { \
	type data; \
	struct name##_cell *next; \
}; \
\
struct name##_queue { \
	unsigned int size; \
	struct name##_cell *queue; \
}; \
\
typedef struct name##_queue name##_queue_t; \
\
name##_queue_t *name##_init_queue(); \
int name##_enqueue(name##_queue_t *q, type new_data); \
int name##_dequeue(name##_queue_t *q, type* old_data); \
int name##_kill_queue(name##_queue_t *q); \
int name##_queue_empty(name##_queue_t *q); \
\
name##_queue_t *name##_init_queue() \
{ \
	name##_queue_t *q = malloc(sizeof(*q)); \
	if (q == NULL) \
		return NULL; \
\
	q->queue = malloc(sizeof(*(q->queue))); \
	if (q->queue == NULL) { \
		free(q); \
		q = NULL; \
		return NULL; \
	} \
	q->queue->next = q->queue; \
	q->size = 0; \
\
	return q; \
} \
\
int name##_enqueue(name##_queue_t *q, type new_data) \
{ \
	struct name##_cell *new_cell = malloc(sizeof(*new_cell)); \
\
	if (new_cell == NULL) return -1; \
\
	new_cell->next = q->queue->next; \
	q->queue->next = new_cell; \
\
	q->queue->data = new_data; \
\
	q->queue = new_cell; \
\
	(q->size)++; \
\
	return 0; \
} \
\
int name##_dequeue(name##_queue_t *q, type* old_data) \
{ \
	if ((q == NULL) || (old_data == NULL)) return -1; \
	if (q->queue->next == q->queue) return -2; \
\
	struct name##_cell *rubbish = q->queue->next; \
\
	*old_data = rubbish->data; \
\
	q->queue->next = rubbish->next; \
\
	free(rubbish); \
	rubbish = NULL; \
\
	(q->size)--; \
\
	return 0; \
} \
\
int name##_kill_queue(name##_queue_t *q) \
{ \
	struct name##_cell tmp; \
\
	while (name##_dequeue(q, &(tmp.data)) == 0); \
	free(q->queue); \
	q->queue = NULL; \
\
	return 0; \
} \
\
int name##_queue_empty(name##_queue_t *q) \
{ \
	return ((q->queue->next) == q->queue); \
}

#endif
