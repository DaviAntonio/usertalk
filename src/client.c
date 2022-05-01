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

#include "settings.h"

int main(int argc, char **argv)
{
	int srv_sock;
	struct sockaddr_in srv_addr;
	int result;
	bool stop = false;
	int opt;

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

	// app loop
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

	while (!stop) {
		prompt = std_prompt;
		printf("%s", prompt);
		if (scanf(" %" MAX_MSG_LEN_STR "[^\n]", msg) != 1) {
			printf("Stopping...\n");
			stop = true;

			printf("Closing...\n");
			close(srv_sock);
			continue;
		}

		// send message to server
		if (send(srv_sock, msg, strlen(msg), 0) == -1) {
			error_at_line(0, errno, __FILE__, __LINE__, "send()");
		}

		memset(msg, 0, MSG_LEN);

		// receive message from server
		result = recv(srv_sock, msg, MSG_LEN, 0);
		if (result <= 0) {
			if (!result) {
				printf("Empty message\n");
				printf("Closing...\n");
				close(srv_sock);
				stop = true;
			} else {
				error_at_line(0, errno, __FILE__, __LINE__,
						"recv()");
			}
		} else {
			char display_msg[MSG_LEN];
			int n;
			if (sscanf(msg,
				"\\SERVERMSG %" MAX_MSG_LEN_STR "s %" MAX_MSG_LEN_STR "[^\n]%n",
				custom_prompt, display_msg, &n) == 2) {
				if (n > 0 && msg[n] == '\0') {
					prompt = custom_prompt;
					printf("%.*s %.*s\n", MSG_LEN, prompt,
							MSG_LEN, display_msg);
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
	}

	return 0;
}
