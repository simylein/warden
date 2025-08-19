const colorDevice = (element, type) => {
	switch (true) {
		case type === 'indoor':
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case type === 'outdoor':
			return element.classList.add('text-sky-600', 'dark:text-sky-400', 'background-sky-50', 'dark:background-sky-950');
	}
};
