#ifndef CLIENT_POINTER_QUEUE_H
#define CLIENT_POINTER_QUEUE_H

#include "generic_queue.h"
#include "server_client.h"
#include <stdbool.h>

typedef struct client *client_pointer_t;

DECLARE_CIRCULAR_QUEUE(client_pointer, client_pointer_t)

int talk_to_group(client_pointer_queue_t *q, struct client *tx)
{
	if (q == NULL)
		return -1;

	if (tx == NULL)
		return -2;

	struct client_pointer_cell *start = q->queue;
	struct client_pointer_cell *p;

	for (p = start->next; p != start; p = p->next) {
		if (p->data->id != tx->id) {
			printf("Client %u talks to %u in room %u\n", tx->id,
					p->data->id, tx->room_id);
			strncpy(p->data->msg, tx->msg, MSG_LEN);
			p->data->force_status = true;
			p->data->status = TO_WRITE;
		}
	}

	tx->force_status = true;
	tx->status = TO_READ;

	return 0;
}

int find_client_by_id(client_pointer_queue_t *q, unsigned int id,
		struct client *found)
{
	if (q == NULL)
		return -1;

	if (found == NULL)
		return -2;

	struct client_pointer_cell *start = q->queue;
	struct client_pointer_cell *p;

	for (p = start->next; p != start; p = p->next) {
		if (p->data->id == id) {
			*found = *(p->data);
			return 0;
		}
	}

	return -3;
}

int remove_client_by_id(client_pointer_queue_t *q, unsigned int id)
{
	if (q == NULL)
		return -1;

	if (client_pointer_queue_empty(q))
		return -2;

	bool found = false;
	for (unsigned int i = 0; i < q->size; i++) {
		struct client cc;
		struct client *c = &cc;
		client_pointer_dequeue(q, &c);
		if (c->id != id) {
			client_pointer_enqueue(q, c);
		} else {
			found = true;
		}
	}

	return found ? 0 : -3;
}

#endif
