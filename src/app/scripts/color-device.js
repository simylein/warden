const colorDevice = (element, type) => {
	switch (true) {
		case type === 'rack':
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case type === 'indoor':
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case type === 'outdoor':
			return element.classList.add('text-sky-600', 'dark:text-sky-400', 'background-sky-50', 'dark:background-sky-950');
	}
};
