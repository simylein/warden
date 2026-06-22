const colorPacketLoss = (element, packetLoss) => {
	switch (true) {
		case packetLoss < 2:
			return element.classList.add('text-green-600', 'dark:text-green-400', 'background-green-50', 'dark:background-green-950');
		case packetLoss >= 2 && packetLoss < 5:
			return element.classList.add('text-lime-600', 'dark:text-lime-400', 'background-lime-50', 'dark:background-lime-950');
		case packetLoss >= 5 && packetLoss < 9:
			return element.classList.add('text-yellow-600', 'dark:text-yellow-400', 'background-yellow-50', 'dark:background-yellow-950');
		case packetLoss >= 9 && packetLoss < 14:
			return element.classList.add('text-amber-600', 'dark:text-amber-400', 'background-amber-50', 'dark:background-amber-950');
		case packetLoss >= 14 && packetLoss < 20:
			return element.classList.add('text-orange-600', 'dark:text-orange-400', 'background-orange-50', 'dark:background-orange-950');
		case packetLoss >= 20:
			return element.classList.add('text-red-600', 'dark:text-red-400', 'background-red-50', 'dark:background-red-950');
	}
};
