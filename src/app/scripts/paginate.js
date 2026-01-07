const pagination = document.getElementById('pagination');

const getLimit = () => {
	const limit = new URLSearchParams(window.location.search).get('limit');
	return +(limit ?? 16);
};

const setLimit = (limit) => {
	const params = new URLSearchParams(window.location.search);
	params.set('limit', limit);
	window.history.replaceState({}, '', `${window.location.pathname}?${params.toString()}`);
};

const getOffset = () => {
	const offset = new URLSearchParams(window.location.search).get('offset');
	return +(offset ?? 0);
};

const setOffset = (offset) => {
	const params = new URLSearchParams(window.location.search);
	params.set('offset', offset);
	window.history.replaceState({}, '', `${window.location.pathname}?${params.toString()}`);
};

const loadingPagination = () => {
	const offset = getOffset();
	const limit = getLimit();
	const page = Math.floor(offset / limit) + 1;
	pagination.children[0].classList.remove('cursor-pointer');
	pagination.children[0].classList.add('cursor-not-allowed', 'opacity-50');
	pagination.children[0].onclick = null;
	loading(pagination.children[1].children[0].children[0], ['h-4'], [], '');
	pagination.children[1].children[0].classList.remove('cursor-pointer');
	pagination.children[1].children[0].onclick = null;
	if (page <= 2) {
		pagination.children[1].children[0].classList.add('hidden');
	} else {
		pagination.children[1].children[0].classList.remove('hidden');
	}
	loading(pagination.children[1].children[1].children[0], ['h-4'], [], '');
	pagination.children[1].children[1].classList.remove('cursor-pointer');
	pagination.children[1].children[1].onclick = null;
	if (page <= 1) {
		pagination.children[1].children[1].classList.add('hidden');
	} else {
		pagination.children[1].children[1].classList.remove('hidden');
	}
	loading(pagination.children[1].children[2].children[0], ['h-4'], [], '');
	loading(pagination.children[1].children[3].children[0], ['h-4'], [], '');
	pagination.children[1].children[3].classList.remove('cursor-pointer');
	pagination.children[1].children[3].onclick = null;
	loading(pagination.children[1].children[4].children[0], ['h-4'], [], '');
	pagination.children[1].children[4].classList.remove('cursor-pointer');
	pagination.children[1].children[4].onclick = null;
	pagination.children[2].classList.remove('cursor-pointer');
	pagination.children[2].classList.add('cursor-not-allowed', 'opacity-50');
	pagination.children[2].onclick = null;
};

const paintPagination = () => {
	const offset = getOffset();
	const limit = getLimit();
	const page = Math.floor(offset / limit) + 1;
	if (page <= 1) {
		pagination.children[0].classList.remove('cursor-pointer');
		pagination.children[0].classList.add('cursor-not-allowed', 'opacity-50');
		pagination.children[0].onclick = null;
	} else {
		pagination.children[0].classList.remove('cursor-not-allowed', 'opacity-50');
		pagination.children[0].classList.add('cursor-pointer');
		pagination.children[0].onclick = previousPage;
	}
	if (page <= 2) {
		pagination.children[1].children[0].classList.add('hidden');
	} else {
		paint(pagination.children[1].children[0].children[0], [], ['h-4'], page - 2);
		pagination.children[1].children[0].classList.remove('hidden');
		pagination.children[1].children[0].classList.add('cursor-pointer');
		pagination.children[1].children[0].onclick = () => {
			setOffset(offset - limit * 2);
			onPaginate();
		};
	}
	if (page <= 1) {
		pagination.children[1].children[1].classList.add('hidden');
	} else {
		paint(pagination.children[1].children[1].children[0], [], ['h-4'], page - 1);
		pagination.children[1].children[1].classList.remove('hidden');
		pagination.children[1].children[1].classList.add('cursor-pointer');
		pagination.children[1].children[1].onclick = () => {
			setOffset(offset - limit);
			onPaginate();
		};
	}
	paint(pagination.children[1].children[2].children[0], [], ['h-4'], page);
	paint(pagination.children[1].children[3].children[0], [], ['h-4'], page + 1);
	pagination.children[1].children[3].classList.add('cursor-pointer');
	pagination.children[1].children[3].onclick = () => {
		setOffset(offset + limit);
		onPaginate();
	};
	paint(pagination.children[1].children[4].children[0], [], ['h-4'], page + 2);
	pagination.children[1].children[4].classList.add('cursor-pointer');
	pagination.children[1].children[4].onclick = () => {
		setOffset(offset + limit * 2);
		onPaginate();
	};
	pagination.children[2].classList.remove('cursor-not-allowed', 'opacity-50');
	pagination.children[2].classList.add('cursor-pointer');
	pagination.children[2].onclick = nextPage;
};

const errorPagination = () => {
	const offset = getOffset();
	const limit = getLimit();
	const page = Math.floor(offset / limit) + 1;
	pagination.children[0].classList.remove('cursor-pointer');
	pagination.children[0].classList.add('cursor-not-allowed', 'opacity-50');
	pagination.children[0].onclick = null;
	error(pagination.children[1].children[0].children[0], ['h-4'], [], '');
	pagination.children[1].children[0].classList.remove('cursor-pointer');
	pagination.children[1].children[0].onclick = null;
	if (page <= 2) {
		pagination.children[1].children[0].classList.add('hidden');
	} else {
		pagination.children[1].children[0].classList.remove('hidden');
	}
	error(pagination.children[1].children[1].children[0], ['h-4'], [], '');
	pagination.children[1].children[1].classList.remove('cursor-pointer');
	pagination.children[1].children[1].onclick = null;
	if (page <= 1) {
		pagination.children[1].children[1].classList.add('hidden');
	} else {
		pagination.children[1].children[1].classList.remove('hidden');
	}
	error(pagination.children[1].children[2].children[0], ['h-4'], [], '');
	error(pagination.children[1].children[3].children[0], ['h-4'], [], '');
	pagination.children[1].children[3].classList.remove('cursor-pointer');
	pagination.children[1].children[3].onclick = null;
	error(pagination.children[1].children[4].children[0], ['h-4'], [], '');
	pagination.children[1].children[4].classList.remove('cursor-pointer');
	pagination.children[1].children[4].onclick = null;
	pagination.children[2].classList.remove('cursor-pointer');
	pagination.children[2].classList.add('cursor-not-allowed', 'opacity-50');
	pagination.children[2].onclick = null;
};

const previousPage = () => {
	const offset = getOffset();
	const limit = getLimit();
	if (offset - limit < 0) {
		setOffset(0);
	} else {
		setOffset(offset - limit);
	}
	onPaginate();
};

const nextPage = () => {
	const offset = getOffset();
	const limit = getLimit();
	setOffset(offset + limit);
	onPaginate();
};
