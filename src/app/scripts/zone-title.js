const loadingZoneTitle = (element) => {
	loading(element, ['w-24', 'h-4.5'], []);
	fade(element.children[0]);
	paint(element.children[0], [], [], '');
};

const paintZoneTitle = (element, response) => {
	paint(element, [], ['w-24', 'h-4.5']);
	if (response.headers.get('zone-name') !== null) {
		fade(element.children[0]);
		paint(element.children[0], [], [], response.headers.get('zone-name'));
	} else {
		paint(element.children[0], [], [], 'null');
		colorNull(element.children[0]);
	}
};

const errorZoneTitle = (element) => {
	error(element, ['w-24', 'h-4.5'], []);
	fade(element.children[0]);
	paint(element.children[0], [], [], '');
};
