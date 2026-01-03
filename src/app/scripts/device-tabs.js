const tabs = document.getElementById('tabs');
const deviceTabs = () => {
	const routes = [
		{ endpoint: '', search: {} },
		{ endpoint: '/readings', search: { range: true } },
		{ endpoint: '/metrics', search: { range: true } },
		{ endpoint: '/buffers', search: { range: true } },
		{ endpoint: '/config', search: {} },
		{ endpoint: '/signals', search: { range: true } },
		{ endpoint: '/uplinks', search: {} },
		{ endpoint: '/downlinks', search: {} },
	];
	const pathname = window.location.pathname.substring(0, 40);
	routes.forEach((route, index) => {
		const params = new URLSearchParams();
		if (route.search.range && typeof getRange === 'function') {
			params.set('range', getRange());
		}
		if (params.size > 0) {
			tabs.children[index].href = `${pathname}${route.endpoint}?${params.toString()}`;
		} else {
			tabs.children[index].href = `${pathname}${route.endpoint}`;
		}
	});
};
