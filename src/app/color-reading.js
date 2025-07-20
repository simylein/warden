const fadeTemperature = (element) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
};

const colorTemperature = (element, temperature) => {
	fadeTemperature(element);
	switch (true) {
		case temperature < -20:
			return element.classList.add('text-fuchsia-600', 'dark:text-fuchsia-400', 'background-fuchsia-50', 'dark:background-fuchsia-950');
		case temperature >= -20 && temperature < -16:
			return element.classList.add('text-purple-600', 'dark:text-purple-400', 'background-purple-50', 'dark:background-purple-950');
		case temperature >= -16 && temperature < -12:
			return element.classList.add('text-violet-600', 'dark:text-violet-400', 'background-violet-50', 'dark:background-violet-950');
		case temperature >= -12 && temperature < -8:
			return element.classList.add('text-indigo-600', 'dark:text-indigo-400', 'background-indigo-50', 'dark:background-indigo-950');
		case temperature >= -8 && temperature < -4:
			return element.classList.add('text-blue-600', 'dark:text-blue-400', 'background-blue-50', 'dark:background-blue-950');
		case temperature >= -4 && temperature < 0:
			return element.classList.add('text-sky-600', 'dark:text-sky-400', 'background-sky-50', 'dark:background-sky-950');
		case temperature >= 0 && temperature < 4:
			return element.classList.add('text-cyan-600', 'dark:text-cyan-400', 'background-cyan-50', 'dark:background-cyan-950');
		case temperature >= 4 && temperature < 8:
			return element.classList.add('text-teal-600', 'dark:text-teal-400', 'background-teal-50', 'dark:background-teal-950');
		case temperature >= 8 && temperature < 12:
			return element.classList.add('text-emerald-600', 'dark:text-emerald-400', 'background-emerald-50', 'dark:background-emerald-950');
		case temperature >= 12 && temperature < 16:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case temperature >= 16 && temperature < 20:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case temperature >= 20 && temperature < 24:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case temperature >= 24 && temperature < 28:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case temperature >= 28 && temperature < 32:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case temperature >= 32 && temperature < 36:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case temperature >= 36:
			return element.classList.add('text-rose-600', 'dark:text-rose-400', 'background-rose-50', 'dark:background-rose-950');
	}
};

const fadeHumidity = (element) => {
	element.classList.forEach((cls) => {
		if (cls.startsWith('text-') || cls.startsWith('dark:text-') || cls.startsWith('background-') || cls.startsWith('dark:background-')) {
			element.classList.remove(cls);
		}
	});
};

const colorHumidity = (element, humidity) => {
	fadeHumidity(element);
	switch (true) {
		case humidity < 20:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case humidity >= 20 && humidity < 30:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case humidity >= 30 && humidity < 35:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case humidity >= 35 && humidity < 40:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case humidity >= 40 && humidity < 45:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case humidity >= 45 && humidity < 50:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case humidity >= 50 && humidity < 55:
			return element.classList.add('text-emerald-600', 'dark:text-emerald-400', 'background-emerald-50', 'dark:background-emerald-950');
		case humidity >= 55 && humidity < 60:
			return element.classList.add('text-teal-600', 'dark:text-teal-400', 'background-teal-50', 'dark:background-teal-950');
		case humidity >= 60 && humidity < 65:
			return element.classList.add('text-cyan-600', 'dark:text-cyan-400', 'background-cyan-50', 'dark:background-cyan-950');
		case humidity >= 65 && humidity < 70:
			return element.classList.add('text-sky-600', 'dark:text-sky-400', 'background-sky-50', 'dark:background-sky-950');
		case humidity >= 70 && humidity < 80:
			return element.classList.add('text-blue-600', 'dark:text-blue-400', 'background-blue-50', 'dark:background-blue-950');
		case humidity >= 80:
			return element.classList.add('text-indigo-600', 'dark:text-indigo-400', 'background-indigo-50', 'dark:background-indigo-950');
	}
};
