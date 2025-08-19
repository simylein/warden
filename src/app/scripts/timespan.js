const timespan = (time, range) => {
	const date = new Date(Number(time) * 1000);

	if (range <= 86400) {
		const hours = date.getHours().toString().padStart(2, '0');
		const minutes = date.getMinutes().toString().padStart(2, '0');
		return `${hours}:${minutes}`;
	} else if (range <= 345600) {
		const days = date.getDate().toString().padStart(2, '0');
		const hours = date.getHours().toString().padStart(2, '0');
		return `${days} ${hours}`;
	} else if (range > 345600) {
		const month = (date.getMonth() + 1).toString().padStart(2, '0');
		const days = date.getDate().toString().padStart(2, '0');
		return `${month}.${days}`;
	}
};
