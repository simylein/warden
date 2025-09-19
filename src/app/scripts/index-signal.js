const indexRssi = (value) => {
	switch (true) {
		case value < -125:
			return 1;
		case value >= -125 && value < -115:
			return 2;
		case value >= -115 && value < -100:
			return 3;
		case value >= -100 && value < -80:
			return 4;
		case value >= -80 && value < -60:
			return 5;
		case value >= -60:
			return 6;
	}
};

const indexSnr = (value) => {
	switch (true) {
		case value < -15:
			return 1;
		case value >= -15 && value < -10:
			return 2;
		case value >= -10 && value < -5:
			return 3;
		case value >= -5 && value < 0:
			return 4;
		case value >= 0 && value < 5:
			return 5;
		case value >= 5:
			return 6;
	}
};
