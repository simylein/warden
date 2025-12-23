const envelope = (collection, items, value) => {
	const target = collection[value];
	items.forEach((item) => {
		if (target.min.abs === null || item[value] < target.min.abs) {
			target.min.abs = item[value];
		}
		if (target.max.abs === null || item[value] > target.max.abs) {
			target.max.abs = item[value];
		}
	});
	if (target.min.abs !== null && target.max.abs !== null) {
		target.min.floor = floor(target.min.abs, target.max.abs - target.min.abs);
		target.max.ceil = ceil(target.max.abs, target.max.abs - target.min.abs);
	}
};

const coordinate = (collection, items, value, time, width) => {
	const target = collection[value];
	const xRange = Number(collection[time].end) - Number(collection[time].start);
	const yRange = target.max.ceil - target.min.floor;
	items.forEach((item) => {
		const xPercent = width - ((Number(item[time]) - Number(collection[time].start)) / xRange) * width;
		const yPercent = yRange === 0 ? 100 : 100 - ((item[value] - target.min.floor) / yRange) * 100;
		target.points.push({ x: xPercent, y: yPercent, v: item[value], d: item?.device?.id ?? null });
	});
};
