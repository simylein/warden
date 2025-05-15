#include "api/init.h"
#include "lib/config.h"
#include "lib/logger.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
		info("available command line flags\n");
		info("--name              -n   name of application            (%s)\n", name);
		info("--address           -a   ip address to bind             (%s)\n", address);
		info("--port              -p   port to listen on              (%hu)\n", port);
		info("--database-file     -df  path to sqlite database file   (%s)\n", database_file);
		info("--database-timeout  -dt  milliseconds to wait for lock  (%hu)\n", database_timeout);
		info("--log-level         -ll  logging verbosity to print     (%s)\n", human_log_level(log_level));
		info("--log-requests      -lq  log incoming requests          (%s)\n", human_bool(log_requests));
		info("--log-responses     -ls  log outgoing response          (%s)\n", human_bool(log_responses));
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

	if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
		info("luna version %hhu.%hhu.%hhu %s\n", major, minor, patch, commit);
		info("written by simylein in c\n");
		exit(0);
	}

	info("starting luna application\n");

	int cf_errors = configure(argc, argv);
	if (cf_errors != 0) {
		fatal("config contains %d errors\n", cf_errors);
		exit(1);
	}
}
