const loadingZoneTitle = (element) => {
	loading(element, ['w-24', 'h-4.5'], [], '');
};

const paintZoneTitle = (element, response) => {
	if (response.headers.get('zone-name') !== null) {
		fade(element);
		paint(element, [], ['w-24', 'h-4.5'], response.headers.get('zone-name'));
	} else {
		paint(element, [], [], 'null');
		colorNull(element);
	}
};

const errorZoneTitle = (element) => {
	error(element, ['w-24', 'h-4.5'], [], '');
};
