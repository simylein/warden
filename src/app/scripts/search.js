const getParam = (key, fallback) => {
	const params = new URLSearchParams(window.location.search).get(key);
	return params ?? fallback;
};

const setParam = (key, value) => {
	const params = new URLSearchParams(window.location.search);
	if (value === null) {
		params.delete(key);
	} else {
		params.set(key, value);
	}
	window.history.replaceState({}, '', `${window.location.pathname}?${params.toString()}`);
};
