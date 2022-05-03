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
