const granularity = (from, to) => {
	const range = to - from;
	switch (true) {
		case range <= 21600:
			return Math.ceil(range / 3600) * 5;
		case range <= 86400:
			return Math.ceil(range / 7200) * 10;
		case range <= 345600:
			return Math.ceil(range / 14400) * 20;
		case range <= 1382400:
			return Math.ceil(range / 28800) * 40;
		case range <= 2764800:
			return Math.ceil(range / 43200) * 60;
		default:
			return null;
	}
};
