#include "../api/email.h"
#include "../api/alert.h"
#include "../api/device.h"
#include "../lib/config.h"
#include "../lib/logger.h"
#include "format.h"
#include <stdio.h>
#include <stdlib.h>

int email_send(octet_t *db, alert_t *alert, device_t *device) {
	char addr[16];
	char from[32];
	char to[32];
	email_t email = {.address = (char *)&addr, .from = (char *)&from, .to = (char *)&to};
	uint16_t status = email_select_one(db, &email);
	if (status != 0) {
		return -1;
	}

	char value[8];
	human_value(&value, alert->field, alert->value);

	char subject[128];
	if (sprintf(subject, "%s %s: %.*s - %s %s %s", name, human_severity(alert->severity), device->name_len, device->name,
							human_field(alert->field), human_edge(alert->edge), value) == -1) {
		error("failed to sprintf subject\n");
		return -1;
	}

	char body[256];
	if (sprintf(body, "%s issued an alert with severity %s on field %s %s %s for device %.*s", name,
							human_severity(alert->severity), human_field(alert->field), human_edge(alert->edge), value, device->name_len,
							device->name) == -1) {
		error("failed to sprintf body\n");
		return -1;
	}

	char command[512];
	if (sprintf(command,
							"swaks --server %.*s --port %hu --from %.*s --to %.*s --header \"subject:%s\" --body \"%s\" >> email.log 2>&1",
							email.address_len, email.address, email.port, email.from_len, email.from, email.to_len, email.to, subject,
							body) == -1) {
		error("failed to sprintf command\n");
		return -1;
	}

	int code = system(command);
	if (code != 0) {
		error("email command failed with exit code %d\n", code);
		return -1;
	}

	return 0;
}
