const floor = (value, range) => {
	switch (true) {
		case range <= 1:
			return Math.floor(value * 10) / 10;
		case range <= 2:
			return Math.floor(value * 5) / 5;
		case range <= 5:
			return Math.floor(value * 2) / 2;
		case range <= 10:
			return Math.floor(value);
		case range <= 20:
			return Math.floor(value / 2) * 2;
		case range <= 50:
			return Math.floor(value / 5) * 5;
		case range > 50:
			return Math.floor(value / 10) * 10;
	}
};

const ceil = (value, range) => {
	switch (true) {
		case range <= 1:
			return Math.ceil(value * 10) / 10;
		case range <= 2:
			return Math.ceil(value * 5) / 5;
		case range <= 5:
			return Math.ceil(value * 2) / 2;
		case range <= 10:
			return Math.ceil(value);
		case range <= 20:
			return Math.ceil(value / 2) * 2;
		case range <= 50:
			return Math.ceil(value / 5) * 5;
		case range > 50:
			return Math.ceil(value / 10) * 10;
	}
};
