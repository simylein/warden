#include "api/init.h"
#include "api/seed.h"
#include "lib/config.h"
#include "lib/error.h"
#include "lib/logger.h"
#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int server_sock;
struct sockaddr_in server_addr;

int main(int argc, char *argv[]) {
	if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
		info("available command line flags\n");
		info("--name              -n   name of application             (%s)\n", name);
		info("--address           -a   ip address to bind              (%s)\n", address);
		info("--port              -p   port to listen on               (%hu)\n", port);
		info("--backlog           -b   backlog allowed on socket       (%hhu)\n", backlog);
		info("--queue-size        -qs  size of clients in queue        (%hhu)\n", queue_size);
		info("--least-workers     -lw  least amount of worker threads  (%hhu)\n", least_workers);
		info("--most-workers      -mw  most amount of worker threads   (%hhu)\n", most_workers);
		info("--database-file     -df  path to sqlite database file    (%s)\n", database_file);
		info("--database-timeout  -dt  milliseconds to wait for lock   (%hu)\n", database_timeout);
		info("--log-level         -ll  logging verbosity to print      (%s)\n", human_log_level(log_level));
		info("--log-requests      -lq  log incoming requests           (%s)\n", human_bool(log_requests));
		info("--log-responses     -ls  log outgoing response           (%s)\n", human_bool(log_responses));
		exit(0);
	}

	if (argc >= 2 && (strcmp(argv[1], "--init") == 0 || strcmp(argv[1], "-i") == 0)) {
		sqlite3 *database;
		if (sqlite3_open_v2(database_file, &database, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
			fatal("failed to open %s because %s\n", database_file, sqlite3_errmsg(database));
			exit(1);
		}

		if (init(database) != 0) {
			fatal("failed to initialise database\n");
			exit(1);
		}

		if (sqlite3_close_v2(database) != SQLITE_OK) {
			fatal("failed to close %s because %s\n", database_file, sqlite3_errmsg(database));
			exit(1);
		}

		exit(0);
	}

	if (argc >= 2 && (strcmp(argv[1], "--seed") == 0 || strcmp(argv[1], "-s") == 0)) {
		sqlite3 *database;
		if (sqlite3_open_v2(database_file, &database, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
			fatal("failed to open %s because %s\n", database_file, sqlite3_errmsg(database));
			exit(1);
		}

		if (seed(database) != 0) {
			fatal("failed to seed database\n");
			exit(1);
		}

		if (sqlite3_close_v2(database) != SQLITE_OK) {
			fatal("failed to close %s because %s\n", database_file, sqlite3_errmsg(database));
			exit(1);
		}

		exit(0);
	}

	if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
		info("luna version %s %s\n", version, commit);
		info("written by simylein in c\n");
		exit(0);
	}

	info("parsing command line flags\n");

	int cf_errors = configure(argc, argv);
	if (cf_errors != 0) {
		fatal("config contains %d errors\n", cf_errors);
		exit(1);
	}

	info("starting luna application\n");

	if ((server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		fatal("failed to create socket because %s\n", errno_str());
		exit(1);
	}

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) == -1) {
		fatal("failed to set socket reuse address because %s\n", errno_str());
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(address);
	server_addr.sin_port = htons(port);

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		fatal("failed to bind to socket because %s\n", errno_str());
		exit(1);
	}

	if (listen(server_sock, backlog) == -1) {
		fatal("failed to listen on socket because %s\n", errno_str());
		exit(1);
	}

	info("listening on %s:%d...\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
}
