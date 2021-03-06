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

#ifndef CLIENT_POINTER_QUEUE_H
#define CLIENT_POINTER_QUEUE_H

#include "generic_queue.h"
#include "server_client.h"
#include <stdbool.h>

typedef struct client *client_pointer_t;

DECLARE_CIRCULAR_QUEUE(client_pointer, client_pointer_t)

int talk_to_group(client_pointer_queue_t *q, struct client *tx, int epoll_fd)
{
	if (q == NULL)
		return -1;

	if (tx == NULL)
		return -2;

	struct client_pointer_cell *start = q->queue;
	struct client_pointer_cell *p;

	struct epoll_event ev;
	int result;

	for (p = start->next; p != start; p = p->next) {
		if (p->data->id != tx->id) {
			printf("Client %u talks to %u in room %u\n", tx->id,
					p->data->id, tx->room_id);
			strncpy(p->data->msg, tx->msg, MSG_LEN);
			p->data->force_status = true;
			p->data->status = TO_WRITE;

			ev.data.ptr = p->data;
			printf("Marking client %u for writing\n", p->data->id);
			ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP;
			result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, p->data->fd,
					&ev);
			if (result == -1) {
				error_at_line(-1, errno, __FILE__, __LINE__,
						"epoll_ctl()");
			}
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
