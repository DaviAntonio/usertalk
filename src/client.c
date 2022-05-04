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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "settings.h"

int epoll_fd;
struct epoll_event events[MAX_EVENTS];

int main(int argc, char **argv)
{
	int srv_sock;
	struct sockaddr_in srv_addr;
	int result;
	bool stop = false;
	int opt;
	int ready_fds;

	if (argc != 3) {
		printf("%s server_ip server_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

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

	char msg[MSG_LEN];
	// TCP socket
	srv_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (srv_sock == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "socket()");
	}

	// Watch if the other end remains connected
	opt = 1;
	if (setsockopt(srv_sock, SOL_SOCKET, SO_KEEPALIVE, &opt,
				sizeof(opt)) == -1) {
		error_at_line(-2, errno, __FILE__, __LINE__,
				"setsockopt(SO_KEEPALIVE)");
	}
	// try to connect to server
	if (connect(srv_sock, (const struct sockaddr*) &srv_addr,
				sizeof(srv_addr)) == -1) {
		error_at_line(-2, errno, __FILE__, __LINE__, "connect()");
	}

	memset(msg, 0, MSG_LEN);

	printf("Connected!\n");

	char *prompt;
	char *std_prompt = "> ";
	char custom_prompt[MSG_LEN];
	prompt = std_prompt;

	// Initialise epoll
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__,
				"epoll_create1()");
	}

	// watch this socket for clients (socket available for read)
	struct epoll_event ev;
	ev.data.fd = srv_sock;
	ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
	result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_sock, &ev);
	if (result == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "epoll_ctl()");
	}

	// watch stdin input
	ev.data.fd = STDIN_FILENO;
	ev.events = EPOLLIN;
	result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
	if (result == -1) {
		error_at_line(-1, errno, __FILE__, __LINE__, "epoll_ctl()");
	}

	// app loop
	while (!stop) {
		// block forever until event or signal
		ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS - 1, -1);

		if (ready_fds == -1) {
			error_at_line(-1, errno, __FILE__, __LINE__,
					"epoll_wait()");
		}

		// which event is ready?
		for (int i = 0; i < ready_fds; i++) {
			// the server socket
			if (events[i].data.fd == srv_sock) {
				if ((events[i].events & EPOLLRDHUP) ||
						(events[i].events & EPOLLHUP)) {
					// server disconnected
					printf("Server disconnected\n");
					printf("Closing...\n");
					close(srv_sock);
					stop = true;
					break;
				} else if (events[i].events & EPOLLIN) {
					// read from server
					memset(msg, 0, MSG_LEN);

					// receive message from server
					result = recv(srv_sock, msg, MSG_LEN, 0);
					if (result <= 0) {
						if (!result) {
							printf("Empty message\n");
							printf("Closing...\n");
							close(srv_sock);
							stop = true;
							break;
						} else {
							error_at_line(0, errno,
									__FILE__,
									__LINE__,
									"recv()");
						}
					} else {
						char display_msg[MSG_LEN];
						int n;
						if (sscanf(msg,
							"\\SERVERMSG %" MAX_MSG_LEN_STR "s %" MAX_MSG_LEN_STR "[^\n]%n",
							custom_prompt, display_msg,
							&n) == 2) {
							if (n > 0 && msg[n] == '\0') {
								prompt = custom_prompt;
								printf("%.*s %.*s\n",
										MSG_LEN,
										prompt,
										MSG_LEN,
										display_msg);
							} else {
								prompt = std_prompt;
								printf("Got invalid command from server: %.*s\n",
										MSG_LEN, msg);
							}
						} else {
							prompt = std_prompt;
							printf("Got invalid command format from server: %.*s\n",
									MSG_LEN, msg);
						}
					}
				} else if (events[i].events & EPOLLOUT) {
					// send message to server
					if (send(srv_sock, msg, strlen(msg), 0)
							== -1) {
						error_at_line(0, errno,
								__FILE__,
								__LINE__,
								"send()");
					}
					// wait to read from server
					ev.data.fd = srv_sock;
					ev.events = EPOLLIN | EPOLLRDHUP
							| EPOLLHUP;
					result = epoll_ctl(epoll_fd,
							EPOLL_CTL_MOD,
							srv_sock,
							&ev);
					if (result == -1) {
						error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
					}
				}
			} else {
				// it's stdin
				prompt = std_prompt;
				if (scanf(" %" MAX_MSG_LEN_STR "[^\n]",
							msg) != 1) {
					printf("Stopping...\n");
					stop = true;

					printf("Closing...\n");
					close(srv_sock);
					// stop scanning the ready fds
					break;
				} else {
					// read correctly
					int c;
					// clear buffered input with an 'A'
					ungetc(41, stdin);
					while((c = getchar()) != '\n' && c != EOF);
					// wait to write to server
					ev.data.fd = srv_sock;
					ev.events = EPOLLOUT | EPOLLRDHUP
							| EPOLLHUP;
					result = epoll_ctl(epoll_fd,
							EPOLL_CTL_MOD,
							srv_sock,
							&ev);
					if (result == -1) {
						error_at_line(-1, errno,
								__FILE__,
								__LINE__,
								"epoll_ctl()");
					}
				}
			}
		}
	}

	return 0;
}
