const tabs = document.getElementById('tabs');
const userTabs = () => {
	const routes = [
		{ endpoint: '', search: {} },
		{ endpoint: '/devices', search: {} },
	];
	const pathname = window.location.pathname.substring(0, 38);
	routes.forEach((route, index) => {
		const params = new URLSearchParams();
		if (params.size > 0) {
			tabs.children[index].href = `${pathname}${route.endpoint}?${params.toString()}`;
		} else {
			tabs.children[index].href = `${pathname}${route.endpoint}`;
		}
	});
};
