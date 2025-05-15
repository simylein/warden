#include "config.h"
#include "error.h"
#include "logger.h"
#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void handle(sqlite3 *database, int *client_sock, struct sockaddr_in *client_addr) {
	char request_buffer[16384];
	char response_buffer[16384];

	time_t timestamp = time(NULL);

	size_t request_length = 0;

	uint8_t packets_received = 0;
	ssize_t bytes_received = recv(*client_sock, request_buffer, sizeof(request_buffer), 0);

	if (bytes_received == -1) {
		error("failed to receive data from client because %s\n", errno_str());
		goto cleanup;
	}
	if (bytes_received == 0) {
		warn("client did not send any data\n");
		goto cleanup;
	}

	trace("received %zd bytes in %hhu packets from %s:%d\n", bytes_received, packets_received, inet_ntoa(client_addr->sin_addr),
				ntohs(client_addr->sin_port));

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	struct timespec stop;
	clock_gettime(CLOCK_MONOTONIC, &stop);

	uint8_t packets_sent = 0;
	ssize_t bytes_sent = send(*client_sock, response_buffer, 0, MSG_NOSIGNAL);

	if (bytes_sent == -1) {
		error("failed to send data to client because %s\n", errno_str());
		goto cleanup;
	}
	if (bytes_sent == 0) {
		warn("server did not send any data\n");
		goto cleanup;
	}

	packets_sent++;

	trace("sent %zd bytes in %hhu packets to %s:%d\n", bytes_sent, packets_sent, inet_ntoa(client_addr->sin_addr),
				ntohs(client_addr->sin_port));

cleanup:
	if (close(*client_sock) == -1) {
		error("failed to close client socket because %s\n", errno_str());
	}
}
