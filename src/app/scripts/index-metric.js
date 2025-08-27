const indexPhotovoltaic = (photovoltaic) => {
	switch (true) {
		case photovoltaic < 1.0:
			return 1;
		case photovoltaic >= 1.0 && photovoltaic < 1.8:
			return 2;
		case photovoltaic >= 1.8 && photovoltaic < 2.4:
			return 3;
		case photovoltaic >= 2.4 && photovoltaic < 2.8:
			return 4;
		case photovoltaic >= 2.8 && photovoltaic < 3.0:
			return 5;
		case photovoltaic >= 3.0:
			return 6;
	}
};

const indexBattery = (battery) => {
	switch (true) {
		case battery < 3.5:
			return 1;
		case battery >= 3.5 && battery < 3.6:
			return 2;
		case battery >= 3.6 && battery < 3.7:
			return 3;
		case battery >= 3.7 && battery < 3.78:
			return 4;
		case battery >= 3.78 && battery < 3.85:
			return 5;
		case battery >= 3.85:
			return 6;
	}
};
