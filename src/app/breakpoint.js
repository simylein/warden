const breakpoint = () => {
	switch (true) {
		case window.innerWidth < 640:
			return 100;
		case window.innerWidth >= 640 && window.innerWidth < 768:
			return 150;
		case window.innerWidth >= 768 && window.innerWidth < 1024:
			return 200;
		case window.innerWidth >= 1024 && window.innerWidth < 1280:
			return 250;
		case window.innerWidth >= 1280:
			return 300;
	}
};
