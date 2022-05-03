#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "settings.h"
#include "room.h"
#include "server_client.h"

#define MAX_EVENTS (100)

int epoll_fd;
struct epoll_event events[MAX_EVENTS];
room_queue_t *rooms;

void remove_server_client(struct client *cl);
void server_loop(int server_sock);
void serve_client(struct client *cl);
void process_client_cmd(struct client *cl);
int broadcast_message(struct client *cl, struct room *room);

void remove_server_client(struct client *cl)
{
	int result;

	printf("Removing client %u\n", cl->id);
	result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cl->fd, NULL);
	if (result == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "epoll_ctl()");
	}
	printf("Closing client %u\n", cl->id);
	close(cl->fd);
	printf("Removing client %u from room %u\n", cl->id, cl->room_id);
	result = remove_client_from_room_by_id(rooms, cl->room_id, cl->id);
	if (result != 0) {
		fprintf(stderr, __FILE__ \
				":" XSTR(__LINE__) \
				" Could not remove client %u from room %u!" \
				" remove_client_from_room_by_id() returned %d\n",
				cl->id, cl->room_id, result);
		exit(EXIT_FAILURE);
	}
	printf("Freeing client %u resources\n", cl->id);
	free(cl);
	cl = NULL;
}

void server_loop(int server_sock)
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int ready_fds;
	int client_sock;
	int result;

	while (1) {
		// block forever until event or signal
		ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS - 1, -1);

		if (ready_fds == -1) {
			error_at_line(-1, errno, __FILE__, __LINE__,
					"epoll_wait()");
		}

		// which event is ready?
		for (int i = 0; i < ready_fds; i++) {
			// the server socket woke up, so a client is ready
			if (events[i].data.fd == server_sock) {
				char addr[INET_ADDRSTRLEN];
				client_sock = accept(server_sock,
					(struct sockaddr *) &client_addr,
					&client_addr_len);
				if (client_sock == -1) {
					error_at_line(0, errno, __FILE__,
							__LINE__, "accept()");
				}
				if (inet_ntop(AF_INET, &(client_addr.sin_addr),
							addr,
							INET_ADDRSTRLEN) == NULL) {
					error_at_line(0, errno, __FILE__,
							__LINE__,
							"inet_ntop()");
				}
				printf("\nClient %s:%u connected\n", addr,
						ntohs(client_addr.sin_port));

				// add client to watch list
				// watch when it is ready to be read
				// watch for abnormalities
				// watch for closed on client
				struct client *cl = malloc(sizeof(*cl));
				if (cl == NULL) {
					fprintf(stderr,
							__FILE__ \
							":" \
							XSTR(__LINE__) \
							" malloc()\n");
					exit(EXIT_FAILURE);
				}
				struct epoll_event event;
				cl->fd = client_sock;
				cl->id = random();
				snprintf(cl->nick, MAX_NICK_LEN, "%u", cl->id);
				cl->addr = client_addr;
				cl->status = TO_READ;
				// check if the room exists
				struct room found_room;
				result = find_room_by_id(rooms, DEFAULT_ROOM_ID,
						&found_room);
				if (result != 0) {
					fprintf(stderr, __FILE__ \
							":" XSTR(__LINE__) \
							" find_room_by_id()" \
							" returned %d\n",
							result);
					fprintf(stderr, "Default room is broken!\n");
					exit(EXIT_FAILURE);
				}

				// insert user on default room
				result = client_pointer_enqueue(found_room.clients, cl);
				if (result != 0) {
					fprintf(stderr, __FILE__ ":" \
							XSTR(__LINE__) \
							" client_pointer_enqueue() returned %d\n",
							result);
					exit(EXIT_FAILURE);
				}
				// cache current room
				cl->room_id = DEFAULT_ROOM_ID;
				printf("Client %u added to default room %u\n",
						cl->id, cl->room_id);
				printf("Default room %u has %u clients\n",
						found_room.id,
						found_room.clients->size);
				event.data.ptr = cl;
				event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
				result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
						cl->fd, &event);
				if (result == -1) {
					error_at_line(-1, errno, __FILE__,
							__LINE__,
							"epoll_ctl()");
				}
				printf("Adding client %u @ %s:%u\n", cl->id,
						addr,
						ntohs(cl->addr.sin_port));
			} else {
				struct client *cl = events[i].data.ptr;

				// something that is not the server socket
				if ((events[i].events & EPOLLRDHUP) ||
						(events[i].events & EPOLLHUP)) {
					// remove from list
					printf("\nA client has left\n");
					remove_server_client(cl);
				} else {
					// try to serve the client
					printf("\nServing client %u\n", cl->id);
					serve_client(cl);

					// setting up the next events to watch
					struct epoll_event ev;
					ev.data.ptr = cl;
					switch (cl->status) {
					case TO_READ:
						printf("Marking client %u for reading\n",
								cl->id);
						ev.events = EPOLLIN | EPOLLRDHUP
							| EPOLLHUP;
						result = epoll_ctl(epoll_fd,
							EPOLL_CTL_MOD,
							cl->fd,
							&ev);
						if (result == -1) {
							error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
						}

						break;
					case TO_WRITE:
						printf("Marking client %u for reading\n",
								cl->id);
						ev.events = EPOLLOUT | EPOLLRDHUP
							| EPOLLHUP;
						result = epoll_ctl(epoll_fd,
							EPOLL_CTL_MOD,
							cl->fd,
							&ev);
						if (result == -1) {
							error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
						}

						break;
					case TO_CLOSE:
						remove_server_client(cl);
						break;
					default:
						printf("Unkown client status\n");
					}
				}

			}

		}

	}

}

void serve_client(struct client *cl) {
	char msg[MSG_LEN];
	int result;
	int client_sock = cl->fd;

	if (cl->status == TO_READ) {
		// get the sent message
		memset(msg, 0, MSG_LEN);
		result = recv(client_sock, msg, MSG_LEN, 0);

		if (result <= 0) {
			if (result == 0) {
				printf("Empty message from client %u\n",
						cl->id);
			} else {
				error_at_line(0, errno, __FILE__, __LINE__,
						"recv()");
			}
			printf("Will mark client %u for removal\n",
					cl->id);
			cl->status = TO_CLOSE;
		} else {
			printf("Client %u said: %.*s\n", cl->id, MSG_LEN, msg);
			strncpy(cl->msg, msg, MAX_MSG_LEN);
			// response will be placed on cl->msg
			process_client_cmd(cl);
			/* Mark to expect a write event if status has not been
			 * explicitly modified by the called command
			 */
			if (cl->force_status == false) {
				printf("Will send: %.*s\n", MSG_LEN, cl->msg);
				printf("Will mark client %u for writing\n",
					cl->id);
				cl->status = TO_WRITE;
			}
		}
	} else if (cl->status == TO_WRITE) {
		// write the stored message
		if (send(client_sock, cl->msg, strlen(cl->msg), 0) == -1) {
			error_at_line(0, errno, __FILE__, __LINE__, "send()");
			printf("Will mark client %u for removal\n",
					cl->id);
			cl->status = TO_CLOSE;
		}
		// mark to expect a read event
		printf("Will mark client %u for reading\n",
				cl->id);
		cl->status = TO_READ;
	}
	// restore force status flag to resume normal behaviour
	cl->force_status = false;
}

void process_client_cmd(struct client *cl)
{
	int result;
	unsigned int ui;
	char cmd[MSG_LEN];
	int n;
	const char help_msg[] = "Available commands: " \
				"(\\HELP" \
				" Shows this text)," \
				"(\\SETNICK new_nickname" \
				" Sets user's nickname to new_nickname" \
				" limited to " MAX_NICK_LEN_STR " characters)," \
				"(\\NEWROOM id" \
				" Creates a new room with id)," \
				"(\\JOINROOM id" \
				" Joins the room with id)," \
				"(\\LEAVEROOM" \
				" Leaves the current room)," \
				"(\\INFO" \
				" Prints current user and room information)";
	const size_t help_msg_len = sizeof(help_msg)/sizeof(help_msg[0]);

	if (sscanf(cl->msg, "\\SETNICK %" MAX_NICK_LEN_STR "s%n", cmd, &n) == 1) {
		// switch user nick
		if (n > 0 && cl->msg[n] == '\0') {
			strncpy(cl->nick, cmd, MAX_NICK_LEN);
			printf("Change user nick to %.*s\n", NICK_LEN, cl->nick);
			snprintf(cl->msg, MAX_MSG_LEN,
					"\\SERVERMSG SERVER> Nick changed to: %.*s",
					NICK_LEN, cl->nick);
		} else {
			snprintf(cl->msg, MAX_MSG_LEN,
					"\\SERVERMSG SERVER> Invalid nick");
		}
	} else if (sscanf(cl->msg, "\\NEWROOM %u", &ui) == 1) {
		// create room with limit
		printf("Create room\n");
	} else if (sscanf(cl->msg, "\\JOINROOM %u", &ui) == 1) {
		// join room with id
		printf("Join room\n");
	} else if (sscanf(cl->msg, "\\%" MAX_MSG_LEN_STR "s%n", cmd, &n) == 1) {
		// commands without arguments
		if (n > 0 && cl->msg[n] == '\0') {
			// process commands
			if (strncmp("INFO", cmd, MSG_LEN) == 0) {
				// print client and room
				struct room found_room;
				struct client found_client;

				result = find_client_in_room_by_id(rooms,
						cl->id, &found_client,
						&found_room);
				if (result == 0) {
					snprintf(cl->msg, MAX_MSG_LEN, "\\SERVERMSG SERVER> User %u aka %.*s @ room %u aka %.*s",
							found_client.id,
							NICK_LEN,
							found_client.nick,
							found_room.id,
							ROOM_NAME_LEN,
							found_room.name);

				} else {
					printf(XSTR(__FILE__) ":" XSTR(__LINE__) " find_client_in_room_by_id() returned %d\n",
							result);
					printf("User %u aka %.*s @ INVALID room %u! Disconnecting...\n", cl->id,
							NICK_LEN, cl->nick, cl->room_id);
					/* client will be disconnected instead
					 * of put to be written */
					cl->force_status = true;
					cl->status = TO_CLOSE;
				}
			} else if (strncmp("LEAVEROOM", cmd, MSG_LEN) == 0) {
				// leave the current room
				printf("Leave room\n");
			} else if (strncmp("HELP", cmd, MSG_LEN) == 0) {
				printf("Help requested\n");
				snprintf(cl->msg, MAX_MSG_LEN,
						"\\SERVERMSG SERVER> %.*s",
						(int) strnlen(help_msg, help_msg_len),
						help_msg);
			} else {
				printf("Invalid simple command\n");
				snprintf(cl->msg, MAX_MSG_LEN,
						"\\SERVERMSG SERVER> Invalid simple command");
			}
		} else {
			// invalid command
			printf("Invalid command\n");
			snprintf(cl->msg, MAX_MSG_LEN,
					"\\SERVERMSG SERVER> Invalid command");
		}
	} else {
		// Broadcast message to room
		// get the client's room
		struct room found_room;
		result = find_room_by_id(rooms, cl->room_id, &found_room);
		if (result == 0) {
			// client is on a valid room
			char new_msg[MSG_LEN];
			snprintf(new_msg, MAX_MSG_LEN,
					"\\SERVERMSG %.*s@%.*s> %.*s", NICK_LEN,
					cl->nick, ROOM_NAME_LEN,
					found_room.name, MSG_LEN, cl->msg);
			strncpy(cl->msg, new_msg, MSG_LEN);
			result = broadcast_message(cl, &found_room);
			if (result != 0) {
				fprintf(stderr, __FILE__ ":" XSTR(__LINE__) \
						"Could not broadcast!" \
						" broadcast_message() returned %d\n",
						result);
			}
		} else {
			printf(__FILE__ ":" XSTR(__LINE__) " find_room_by_id() returned %d\n",
					result);
			printf("User %u aka %.*s @ INVALID room %u! Disconnecting...\n", cl->id,
					NICK_LEN, cl->nick, cl->room_id);
			/* client will be disconnected instead
			 * of put to be written */
			cl->force_status = true;
			cl->status = TO_CLOSE;
		}
	}
}

int broadcast_message(struct client *cl, struct room *room)
{
	// room must be validated beforehand
	return talk_to_group(room->clients, cl);
}

int main(int argc, char **argv)
{
	int sock;
	struct sockaddr_in srv_addr;
	int opt;
	int result;

	if (argc != 3) {
		printf("%s server_ip server_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Server started!\n");

	// initialise random
	srandom(getpid());

	// server's address
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	result = inet_pton(AF_INET, argv[1], &(srv_addr.sin_addr));
	if (result <= 0) {
		if (!result) {
			fprintf(stderr, "'%s' is not a valid address\n",
					argv[1]);
			exit(EXIT_FAILURE);
		} else {
			error_at_line(-1, errno, __FILE__, __LINE__,
					"inet_pton()");
		}
	}
	if (sscanf(argv[2], "%hu", &(srv_addr.sin_port)) != 1) {
		fprintf(stderr, "'%s' is an invalid port\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	// TCP socket
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "socket()");
	}

	/* We wish to reuse the socket
	 * SO_REUSEADDR allows the validation of the address to be bound to the
	 * socket to reuse local addresses without waiting for a timeout
	 * All sockets from AF_INET (TCP, UDP) will be able to reuse the address
	 * unless there is a listening socket bound to it
	 */
	opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt,
				sizeof(opt)) == -1) {
		error_at_line(-2, errno, __FILE__, __LINE__,
				"setsockpt(SO_REUSEADDR)");
	}
	// Watch if the other end remains connected
	opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt,
				sizeof(opt)) == -1) {
		error_at_line(-2, errno, __FILE__, __LINE__,
				"setsockopt(SO_KEEPALIVE)");
	}

	// bind address to socket
	if (bind(sock, (const struct sockaddr *) &srv_addr,
				sizeof(srv_addr)) == -1) {
		error_at_line(-2, errno, __FILE__, __LINE__, "bind()");
	}


	/* make the socket a listening one (passive socket)
	 * this makes the socket suitable for accepting any connection request
	 * 10 clients will be placed in the pending connections queue. If more
	 * clients try to connect, they may receive an error or attempt a
	 * retransmission
	 */
	if (listen(sock, 10) == -1) {
		error_at_line(-3, errno, __FILE__, __LINE__, "listen()");
	}

	printf("Listening on %s:%s\n", argv[1], argv[2]);

	// Initialise epoll
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__,
				"epoll_create1()");
	}
	// watch this socket for clients (socket available for read)
	struct epoll_event event;
	event.data.fd = sock;
	event.events = EPOLLIN;
	result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event);
	if (result == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "epoll_ctl()");
	}

	// Initialise rooms' queue
	rooms = room_init_queue();
	if (rooms == NULL) {
		fprintf(stderr, "Could not start queue for rooms\n");
		return -4;
	}

	// create the public default room
	struct room default_room;
	memset(&default_room, 0, sizeof(default_room));
	default_room.id = DEFAULT_ROOM_ID;
	default_room.limit = UINT_MAX;
	strncpy(default_room.name, "Public lobby", ROOM_NAME_LEN);
	default_room.clients = client_pointer_init_queue();
	if (default_room.clients == NULL) {
		fprintf(stderr, "Could not start clients queue for default room\n");
		return -5;
	}

	if (room_enqueue(rooms, default_room) != 0) {
		fprintf(stderr, "Could not add default room\n");
		return -6;
	}

	// begin server loop
	server_loop(sock);

	close(sock);

	room_kill_queue(rooms);

	return 0;
}
