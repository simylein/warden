const tabs = document.getElementById('tabs');
const zoneTabs = () => {
	const routes = [
		{ endpoint: '', search: {} },
		{ endpoint: '/readings', search: { range: true } },
		{ endpoint: '/metrics', search: { range: true } },
	];
	const pathname = window.location.pathname.substring(0, 38);
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
