const severity = (severity) => {
	switch (true) {
		case severity === 0:
			return 'critical';
		case severity === 1:
			return 'warning';
		case severity === 2:
			return 'notice';
		default:
			return 'unknown';
	}
};

const field = (field) => {
	switch (true) {
		case field === 0:
			return 'temperature';
		case field === 1:
			return 'humidity';
		case field === 2:
			return 'photovoltaic';
		case field === 3:
			return 'battery';
		case field === 4:
			return 'delay';
		case field === 5:
			return 'level';
		default:
			return 'unknown';
	}
};

const value = (field, value) => {
	switch (true) {
		case field === 0:
			return value / 100;
		case field === 1:
			return value / 100;
		case field === 2:
			return value / 1000;
		case field === 3:
			return value / 1000;
		case field === 4:
			return value;
		case field === 5:
			return value;
		default:
			return value;
	}
};

const status = (resolvedAt) => {
	if (resolvedAt === null) {
		return 'ongoing';
	}
	return 'resolved';
};
