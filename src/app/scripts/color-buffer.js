const colorDelay = (element, delay) => {
	switch (true) {
		case delay < 240:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case delay >= 240 && delay < 900:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case delay >= 900 && delay < 3600:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case delay >= 3600 && delay < 21600:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case delay >= 21600 && delay < 86400:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case delay >= 86400:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
	}
};

const colorLevel = (element, level) => {
	switch (true) {
		case level < 4:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case level >= 4 && level < 16:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case level >= 16 && level < 64:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case level >= 64 && level < 256:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case level >= 256 && level < 1024:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case level >= 1024:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
	}
};
