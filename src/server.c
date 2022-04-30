#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "settings.h"

#define MAX_EVENTS (100)

enum client_status {
	TO_READ,
	TO_WRITE,
	TO_CLOSE
};

struct client {
	int fd;
	unsigned int id;
	struct sockaddr_in addr;
	char msg[MSG_LEN];
	enum client_status status;
};

int epoll_fd;
struct epoll_event events[MAX_EVENTS];

void server_loop(int server_sock);
void serve_client(struct client *cl);

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

				// add client to list
				// watch when it is ready to be read
				// watch for abnormalities
				// watch for closed on client
				struct client *cl = malloc(sizeof(*cl));
				if (cl == NULL) {
					fprintf(stderr,
							__FILE__ \
							" " \
							XSTR(__LINE__) \
							" malloc()\n");
					exit(EXIT_FAILURE);
				}
				struct epoll_event event;
				cl->fd = client_sock;
				cl->id = random();
				cl->addr = client_addr;
				cl->status = TO_READ;
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
					printf("Removing client %u\n",
							cl->id);
					result = epoll_ctl(epoll_fd,
							EPOLL_CTL_DEL,
							cl->fd,
							NULL);
					if (result == -1) {
						error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
					}
					printf("Closing client %u\n",
							cl->id);
					close(cl->fd);
					free(cl);
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
						printf("Removing client %u\n",
							cl->id);
						result = epoll_ctl(epoll_fd,
							EPOLL_CTL_DEL,
							cl->fd,
							NULL);
						if (result == -1) {
							error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
						}
						printf("Closing client %u\n",
							cl->id);
						close(cl->fd);
						free(cl);
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
			// TODO: run stuff...
			// prepare a message
			memset(cl->msg, 0, MSG_LEN);
			strncat(cl->msg, "You said: ", MSG_LEN - strlen(cl->msg) - 1);
			strncat(cl->msg, msg, MSG_LEN - strlen(cl->msg) - 1);
			// mark to expect a write event
			printf("Will mark client %u for writing\n",
					cl->id);
			cl->status = TO_WRITE;
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

	// begin server loop
	server_loop(sock);

	close(sock);

	return 0;
}
