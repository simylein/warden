const indexRssi = (rssi, sf) => {
	const floor = -116 - (sf - 6) * 2.25;
	const width = 9 + (sf - 6) * 0.25;
	switch (true) {
		case rssi < floor + width * 1:
			return 1;
		case rssi >= floor + width * 1 && rssi < floor + width * 2:
			return 2;
		case rssi >= floor + width * 2 && rssi < floor + width * 3:
			return 3;
		case rssi >= floor + width * 3 && rssi < floor + width * 4:
			return 4;
		case rssi >= floor + width * 4 && rssi < floor + width * 5:
			return 5;
		case rssi >= floor + width * 5:
			return 6;
	}
};

const indexSnr = (snr, sf) => {
	const floor = -3 - (sf - 6) * 2.25;
	const width = 2 + (sf - 6) * 0.25;
	switch (true) {
		case snr < floor + width * 1:
			return 1;
		case snr >= floor + width * 1 && snr < floor + width * 2:
			return 2;
		case snr >= floor + width * 2 && snr < floor + width * 3:
			return 3;
		case snr >= floor + width * 3 && snr < floor + width * 4:
			return 4;
		case snr >= floor + width * 4 && snr < floor + width * 5:
			return 5;
		case snr >= floor + width * 5:
			return 6;
	}
};
