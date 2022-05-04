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

#ifndef ROOM_H
#define ROOM_H

#include "generic_queue.h"
#include "client_pointer_queue.h"

#define _DEFAULT_ROOM_ID 1
#define DEFAULT_ROOM_ID (_DEFAULT_ROOM_ID)

struct room {
	unsigned int id;
	unsigned int limit;
	char name[ROOM_NAME_LEN];
	client_pointer_queue_t *clients;
};

DECLARE_CIRCULAR_QUEUE(room, struct room)

int find_client_in_room_by_id(room_queue_t *q, unsigned int client_id,
		struct client *client, struct room *room)
{
	if (q == NULL)
		return -1;

	if (client == NULL)
		return -2;

	if (room == NULL)
		return -3;

	struct room_cell *start = q->queue;
	struct room_cell *p;

	for (p = start->next; p != start; p = p->next) {
		if (find_client_by_id(p->data.clients, client_id, client) == 0) {
			*room = p->data;
			return 0;
		}
	}

	return -4;
}

int find_room_by_id(room_queue_t *q, unsigned int id, struct room *found)
{
	if (q == NULL)
		return -1;

	if (room_queue_empty(q))
		return -2;

	struct room_cell *start = q->queue;
	struct room_cell *p;
	for (p = start->next; p != start; p = p->next) {
		if (p->data.id == id) {
			*found = p->data;
			return 0;
		}
	}

	return -3;
}

int remove_client_from_room_by_id(room_queue_t *q, unsigned int room_id,
		unsigned int client_id)
{
	if (q == NULL)
		return -1;

	struct room found;
	if (find_room_by_id(q, room_id, &found) == 0) {
		return remove_client_by_id(found.clients, client_id);
	} else {
		return -2;
	}
}

#endif
