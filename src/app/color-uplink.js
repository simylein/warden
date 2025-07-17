const colorRssi = (element, rssi) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
	switch (true) {
		case rssi < -125:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case rssi >= -125 && rssi < -115:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case rssi >= -115 && rssi < -100:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case rssi >= -100 && rssi < -80:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case rssi >= -80 && rssi < -60:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case rssi >= -60:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
	}
};

const colorSnr = (element, snr) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
	switch (true) {
		case snr < -15:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case snr >= -15 && snr < -10:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case snr >= -10 && snr < -5:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case snr >= -5 && snr < 0:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case snr >= 0 && snr < 5:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case snr >= 5:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
	}
};

const colorSf = (element, sf) => {
	switch (true) {
		case sf === 7:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case sf === 8:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case sf === 9:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case sf === 10:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case sf === 11:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case sf === 12:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
	}
};
