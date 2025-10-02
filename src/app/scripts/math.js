const floor = (value, range) => {
	switch (true) {
		case range <= 0.4:
			return Math.floor(value * 100) / 100;
		case range <= 0.8:
			return Math.floor(value * 50) / 50;
		case range <= 2:
			return Math.floor(value * 20) / 20;
		case range <= 4:
			return Math.floor(value * 10) / 10;
		case range <= 8:
			return Math.floor(value * 5) / 5;
		case range <= 20:
			return Math.floor(value * 2) / 2;
		case range > 20:
			return Math.floor(value);
	}
};

const ceil = (value, range) => {
	switch (true) {
		case range <= 0.4:
			return Math.ceil(value * 100) / 100;
		case range <= 0.8:
			return Math.ceil(value * 50) / 50;
		case range <= 2:
			return Math.ceil(value * 20) / 20;
		case range <= 4:
			return Math.ceil(value * 10) / 10;
		case range <= 8:
			return Math.ceil(value * 5) / 5;
		case range <= 20:
			return Math.ceil(value * 2) / 2;
		case range > 20:
			return Math.ceil(value);
	}
};
