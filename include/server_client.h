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

#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include <stdbool.h>
#include <sys/socket.h>

#include "settings.h"

enum client_status {
	TO_READ,
	TO_WRITE,
	TO_CLOSE
};

struct client {
	int fd;
	unsigned int id;
	unsigned int room_id;
	char nick[NICK_LEN];
	struct sockaddr_in addr;
	char msg[MSG_LEN];
	enum client_status status;
	bool force_status;
};

#endif
