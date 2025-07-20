const fadePhotovoltaic = (element) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
};

const colorPhotovoltaic = (element, photovoltaic) => {
	fadePhotovoltaic(element);
	switch (true) {
		case photovoltaic < 0.5:
			return element.classList.add('text-blue-600', 'dark:text-blue-400', 'background-blue-50', 'dark:background-blue-950');
		case photovoltaic >= 0.5 && photovoltaic < 1.0:
			return element.classList.add('text-sky-600', 'dark:text-sky-400', 'background-sky-50', 'dark:background-sky-950');
		case photovoltaic >= 1.0 && photovoltaic < 1.5:
			return element.classList.add('text-cyan-600', 'dark:text-cyan-400', 'background-cyan-50', 'dark:background-cyan-950');
		case photovoltaic >= 1.5 && photovoltaic < 2.0:
			return element.classList.add('text-teal-600', 'dark:text-teal-400', 'background-teal-50', 'dark:background-teal-950');
		case photovoltaic >= 2.0 && photovoltaic < 3.0:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case photovoltaic >= 3.0:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
	}
};

const fadeBattery = (element) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
};

const colorBattery = (element, battery) => {
	fadeBattery(element);
	switch (true) {
		case battery < 3.5:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case battery >= 3.5 && battery < 3.6:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case battery >= 3.6 && battery < 3.7:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case battery >= 3.7 && battery < 3.78:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case battery >= 3.78 && battery < 3.85:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case battery >= 3.85:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
	}
};
