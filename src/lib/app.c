#include "../api/router.h"
#include "config.h"
#include "error.h"
#include "format.h"
#include "logger.h"
#include "request.h"
#include "response.h"
#include "utils.h"
#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void handle(sqlite3 *database, int *client_sock, struct sockaddr_in *client_addr) {
	char request_buffer[16384];
	char response_buffer[16384];

	if (setsockopt(*client_sock, SOL_SOCKET, SO_RCVTIMEO, &(struct timeval){.tv_sec = receive_timeout, .tv_usec = 0},
								 sizeof(struct timeval)) == -1) {
		error("failed to set socket receive timeout because %s\n", errno_str());
		goto cleanup;
	}

	if (setsockopt(*client_sock, SOL_SOCKET, SO_SNDTIMEO, &(struct timeval){.tv_sec = send_timeout, .tv_usec = 0},
								 sizeof(struct timeval)) == -1) {
		error("failed to set socket send timeout because %s\n", errno_str());
		goto cleanup;
	}

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

	packets_received++;

	const char *length_index = strncasestrn(request_buffer, (size_t)bytes_received, "content-length:", 15);
	const char *body_index = strncasestrn(request_buffer, (size_t)bytes_received, "\r\n\r\n", 4);

	if (length_index && body_index) {
		length_index += 15;
		body_index += 4;

		const size_t content_length = (size_t)atoi(length_index);
		if (content_length > 0) {
			request_length = (size_t)(body_index - request_buffer) + content_length;
		}
	}

	while ((size_t)bytes_received < request_length) {
		if (packets_received + 1 > receive_packets) {
			warn("packets received %hhu exceeds allowed packets %hhu\n", packets_received, receive_packets);
			break;
		}

		if (request_length > sizeof(request_buffer)) {
			warn("request length %zu exceeds buffer length %zu\n", request_length, sizeof(request_buffer));
			break;
		}

		ssize_t further_bytes_received =
				recv(*client_sock, &request_buffer[bytes_received], sizeof(request_buffer) - (size_t)bytes_received, 0);

		if (further_bytes_received == -1) {
			error("failed to receive further data from client because %s\n", errno_str());
			goto cleanup;
		}
		if (further_bytes_received == 0) {
			warn("client did not send any further data\n");
			goto cleanup;
		}

		bytes_received += further_bytes_received;
		packets_received++;
	}

	trace("received %zd bytes in %hhu packets from %s:%d\n", bytes_received, packets_received, inet_ntoa(client_addr->sin_addr),
				ntohs(client_addr->sin_port));

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	struct request_t reqs;
	struct response_t resp;
	request_init(&reqs);
	response_init(&resp, response_buffer);

	char bytes_buffer[8];
	human_bytes(&bytes_buffer, (size_t)bytes_received);

	request(request_buffer, (size_t)bytes_received, &reqs, &resp);
	trace("method %hhub pathname %hhub search %hub header %hub body %zub\n", reqs.method_len, reqs.pathname_len, reqs.search_len,
				reqs.header_len, reqs.body_len);
	req("%.*s %.*s %s\n", (int)reqs.method_len, reqs.method, (int)reqs.pathname_len, reqs.pathname, bytes_buffer);

	route(database, &reqs, &resp);

	size_t response_length = response(response_buffer, &reqs, &resp);

	struct timespec stop;
	clock_gettime(CLOCK_MONOTONIC, &stop);

	char duration_buffer[8];
	human_duration(&duration_buffer, &start, &stop);
	human_bytes(&bytes_buffer, response_length);

	res("%d %s %s\n", resp.status, duration_buffer, bytes_buffer);
	trace("head %hhub header %hub body %zub\n", resp.head_len, resp.header_len, resp.body_len);

	uint8_t packets_sent = 0;
	ssize_t bytes_sent = send(*client_sock, response_buffer, response_length, MSG_NOSIGNAL);

	if (bytes_sent == -1) {
		error("failed to send data to client because %s\n", errno_str());
		goto cleanup;
	}
	if (bytes_sent == 0) {
		warn("server did not send any data\n");
		goto cleanup;
	}

	packets_sent++;

	while ((size_t)bytes_sent < response_length) {
		if (packets_sent + 1 > send_packets) {
			warn("packets sent %hhu exceeds allowed packets %hhu\n\n", packets_sent, send_packets);
			break;
		}

		if (response_length > sizeof(response_buffer)) {
			warn("response length %zu exceeds buffer length %zu\n", response_length, sizeof(response_buffer));
			break;
		}

		ssize_t further_bytes_sent =
				send(*client_sock, &response_buffer[bytes_sent], sizeof(response_buffer) - (size_t)bytes_sent, MSG_NOSIGNAL);

		if (further_bytes_sent == -1) {
			error("failed to send further data to client because %s\n", errno_str());
			goto cleanup;
		}
		if (further_bytes_sent == 0) {
			warn("server did not send any further data\n");
			goto cleanup;
		}

		bytes_sent += further_bytes_sent;
		packets_sent++;
	}

	trace("sent %zd bytes in %hhu packets to %s:%d\n", bytes_sent, packets_sent, inet_ntoa(client_addr->sin_addr),
				ntohs(client_addr->sin_port));

cleanup:
	if (close(*client_sock) == -1) {
		error("failed to close client socket because %s\n", errno_str());
	}
}
