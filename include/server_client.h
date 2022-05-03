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
