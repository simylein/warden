const tabs = document.getElementById('tabs');
const deviceTabs = () => {
	const routes = [
		{ endpoint: '', search: {} },
		{ endpoint: '/readings', search: { range: true, from: true, to: true } },
		{ endpoint: '/metrics', search: { range: true, from: true, to: true } },
		{ endpoint: '/buffers', search: { range: true, from: true, to: true } },
		{ endpoint: '/config', search: {} },
		{ endpoint: '/radio', search: {} },
		{ endpoint: '/signals', search: { range: true, from: true, to: true } },
		{ endpoint: '/uplinks', search: {} },
		{ endpoint: '/downlinks', search: {} },
	];
	const pathname = window.location.pathname.substring(0, 24);
	routes.forEach((route, index) => {
		const params = new URLSearchParams();
		if (route.search.range && typeof getRange === 'function') {
			const range = getRange();
			if (range === null) {
				params.delete('range');
			} else {
				params.set('range', range);
			}
		}
		if (route.search.from && typeof getFrom === 'function') {
			const from = getFrom();
			if (from === null) {
				params.delete('from');
			} else {
				params.set('from', from);
			}
		}
		if (route.search.to && typeof getTo === 'function') {
			const to = getTo();
			if (to === null) {
				params.delete('to');
			} else {
				params.set('to', getTo());
			}
		}
		if (params.size > 0) {
			tabs.children[index].href = `${pathname}${route.endpoint}?${params.toString()}`;
		} else {
			tabs.children[index].href = `${pathname}${route.endpoint}`;
		}
	});
};
