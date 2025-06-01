#include "../lib/logger.h"
#include "user.h"
#include <sqlite3.h>

int seed_user(sqlite3 *database) {
	uint8_t ids[4][16];
	user_t users[4] = {
			{.id = &ids[0], .username = "alice", .username_len = 5, .password = ".go4Alice", .password_len = 9},
			{.id = &ids[1], .username = "bob", .username_len = 3, .password = ".go4Bob", .password_len = 7},
			{.id = &ids[2], .username = "charlie", .username_len = 7, .password = ".go4Charlie", .password_len = 11},
			{.id = &ids[3], .username = "dave", .username_len = 4, .password = ".go4Dave", .password_len = 8},
	};

	for (uint8_t index = 0; index < sizeof(users) / sizeof(user_t); index++) {
		if (user_insert(database, &users[index]) != 0) {
			return -1;
		}
	}
	info("seeded table user\n");

	return 0;
}

int seed(sqlite3 *database) {
	if (seed_user(database) == -1) {
		return -1;
	}

	return 0;
}
