const dewpointing = (temperature, humidity) => {
	const alpha = 17.625;
	const bravo = 243.04;
	const gamma = (alpha * temperature) / (bravo + temperature) + Math.log(humidity / 100);
	return (bravo * gamma) / (alpha - gamma);
};
