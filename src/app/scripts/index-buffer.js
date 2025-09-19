const indexDelay = (value) => {
	switch (true) {
		case value < 240:
			return 1;
		case value >= 240 && value < 900:
			return 2;
		case value >= 900 && value < 3600:
			return 3;
		case value >= 3600 && value < 21600:
			return 4;
		case value >= 21600 && value < 86400:
			return 5;
		case value >= 86400:
			return 6;
	}
};

const indexLevel = (value) => {
	switch (true) {
		case value < 4:
			return 1;
		case value >= 4 && value < 16:
			return 2;
		case value >= 16 && value < 64:
			return 3;
		case value >= 64 && value < 256:
			return 4;
		case value >= 256 && value < 1024:
			return 5;
		case value >= 1024:
			return 6;
	}
};
