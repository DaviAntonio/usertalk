#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "settings.h"

void server_loop(int server_sock);
void serve_client(int client_sock, struct sockaddr_in client_addr);

void server_loop(int server_sock)
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (1) {
		int client_sock = accept(server_sock,
				(struct sockaddr *) &client_addr,
				&client_addr_len);
		if (client_sock == -1) {
			error_at_line(0, errno, __FILE__, __LINE__,
					"accept()");
		}

		serve_client(client_sock, client_addr);

		close(client_sock);
	}

}

void serve_client(int client_sock, struct sockaddr_in client_addr) {
	char addr[INET_ADDRSTRLEN];
	char msg[MSG_LEN];
	char new_msg[MSG_LEN];
	int result;

	if (inet_ntop(AF_INET, &(client_addr.sin_addr), addr,
				INET_ADDRSTRLEN) == NULL) {
		error_at_line(0, errno, __FILE__, __LINE__, "inet_ntop()");
	}
	printf("\nClient %s:%u connected\n", addr, ntohs(client_addr.sin_port));

	// get the sent message
	memset(msg, 0, MSG_LEN);
	result = recv(client_sock, msg, MSG_LEN, 0);

	if (result <= 0) {
		if (result == 0) {
			printf("Empty message\n");
		} else {
			error_at_line(0, errno, __FILE__, __LINE__, "recv()");
		}
	} else {
		printf("Client said %.*s\n", MSG_LEN, msg);
	}

	// send a message
	memset(new_msg, 0, MSG_LEN);
	strncat(new_msg, "Client said: ", MSG_LEN - strlen(new_msg) - 1);
	strncat(new_msg, msg, MSG_LEN - strlen(new_msg) - 1);
	if (send(client_sock, new_msg, strlen(new_msg), 0) == -1) {
		error_at_line(0, errno, __FILE__, __LINE__, "send()");
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

	// begin server loop
	server_loop(sock);

	close(sock);

	return 0;
}
