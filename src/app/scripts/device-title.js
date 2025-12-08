const loadingDeviceTitle = (element) => {
	loading(element, ['w-56', 'h-4.5'], []);
	fade(element.children[0]);
	paint(element.children[0], [], [], '');
	fade(element.children[1]);
	paint(element.children[1], [], [], '');
	fade(element.children[2]);
	paint(element.children[2], [], [], '');
};

const paintDeviceTitle = (element, response) => {
	paint(element, [], ['w-56', 'h-4.5']);
	if (response.headers.get('device-name') !== null) {
		fade(element.children[0]);
		paint(element.children[0], [], [], response.headers.get('device-name'));
	} else {
		paint(element.children[0], [], [], 'null');
		colorNull(element.children[0]);
	}
	paint(element.children[1], [], [], ' - ');
	if (response.headers.get('device-zone-name') !== null) {
		fade(element.children[2]);
		paint(element.children[2], [], [], response.headers.get('device-zone-name'));
	} else {
		paint(element.children[2], [], [], 'null');
		colorNull(element.children[2]);
	}
};

const errorDeviceTitle = (element) => {
	error(element, ['w-56', 'h-4.5'], []);
	fade(element.children[0]);
	paint(element.children[0], [], [], '');
	fade(element.children[1]);
	paint(element.children[1], [], [], '');
	fade(element.children[2]);
	paint(element.children[2], [], [], '');
};
