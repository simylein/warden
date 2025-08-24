const indexTemperature = (temperature) => {
	switch (true) {
		case temperature < -20:
			return 1;
		case temperature >= -20 && temperature < -16:
			return 2;
		case temperature >= -16 && temperature < -12:
			return 3;
		case temperature >= -12 && temperature < -8:
			return 4;
		case temperature >= -8 && temperature < -4:
			return 5;
		case temperature >= -4 && temperature < 0:
			return 6;
		case temperature >= 0 && temperature < 4:
			return 7;
		case temperature >= 4 && temperature < 8:
			return 8;
		case temperature >= 8 && temperature < 12:
			return 9;
		case temperature >= 12 && temperature < 16:
			return 10;
		case temperature >= 16 && temperature < 20:
			return 11;
		case temperature >= 20 && temperature < 24:
			return 12;
		case temperature >= 24 && temperature < 28:
			return 13;
		case temperature >= 28 && temperature < 32:
			return 14;
		case temperature >= 32 && temperature < 36:
			return 15;
		case temperature >= 36:
			return 16;
	}
};

const indexHumidity = (humidity) => {
	switch (true) {
		case humidity < 20:
			return 1;
		case humidity >= 20 && humidity < 30:
			return 2;
		case humidity >= 30 && humidity < 35:
			return 3;
		case humidity >= 35 && humidity < 40:
			return 4;
		case humidity >= 40 && humidity < 45:
			return 5;
		case humidity >= 45 && humidity < 50:
			return 6;
		case humidity >= 50 && humidity < 55:
			return 7;
		case humidity >= 55 && humidity < 60:
			return 8;
		case humidity >= 60 && humidity < 65:
			return 9;
		case humidity >= 65 && humidity < 70:
			return 10;
		case humidity >= 70 && humidity < 80:
			return 11;
		case humidity >= 80:
			return 12;
	}
};
