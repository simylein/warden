const breakpoint = () => {
	switch (true) {
		case window.innerWidth < 384:
			return 92;
		case window.innerWidth >= 384 && window.innerWidth < 512:
			return 158;
		case window.innerWidth >= 512 && window.innerWidth < 640:
			return 224;
		case window.innerWidth >= 640 && window.innerWidth < 768:
			return 290;
		case window.innerWidth >= 768 && window.innerWidth < 1024:
			return 344;
		case window.innerWidth >= 1024 && window.innerWidth < 1280:
			return 476;
		case window.innerWidth >= 1280 && window.innerWidth < 1536:
			return 596;
		case window.innerWidth >= 1536 && window.innerWidth < 1792:
			return 730;
		case window.innerWidth >= 1792 && window.innerWidth < 2048:
			return 864;
		case window.innerWidth >= 2048:
			return 1000;
	}
};
