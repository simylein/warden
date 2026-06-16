const colorSeverity = (element, severity) => {
	switch (true) {
		case severity === 0:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case severity === 1:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case severity === 2:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
	}
};

const colorEdge = (element, edge) => {
	switch (true) {
		case edge === 0:
			return element.classList.add('text-blue-600', 'dark:text-blue-400', 'background-blue-50', 'dark:background-blue-950');
		case edge === 1:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
	}
};

const colorValue = (element, field, value) => {
	switch (true) {
		case field === 0:
			colorTemperature(element, value);
			element.innerText = value.toFixed(2);
			break;
		case field === 1:
			colorHumidity(element, value);
			element.innerText = value.toFixed(2);
			break;
		case field === 2:
			colorDewpoint(element, value);
			element.innerText = value.toFixed(2);
			break;
		case field === 3:
			colorPhotovoltaic(element, value);
			element.innerText = value.toFixed(3);
			break;
		case field === 4:
			colorBattery(element, value);
			element.innerText = value.toFixed(3);
			break;
		case field === 5:
			colorDelay(element, value);
			element.innerText = duration(value);
			break;
		case field === 6:
			colorLevel(element, value);
			element.innerText = value;
			break;
	}
};

const colorStatus = (element, status) => {
	switch (true) {
		case status === 'ongoing':
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
		case status === 'resolved':
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
	}
};
