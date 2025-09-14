const breakpoint = (width) => {
	switch (true) {
		case width < 128:
			return 58;
		case width >= 128 && width < 192:
			return 83;
		case width >= 192 && width < 256:
			return 116;
		case width >= 256 && width < 320:
			return 150;
		case width >= 320 && width < 384:
			return 183;
		case width >= 384 && width < 512:
			return 233;
		case width >= 512 && width < 640:
			return 300;
		case width >= 640 && width < 768:
			return 366;
		case width >= 768 && width < 1024:
			return 466;
		case width >= 1024 && width < 1280:
			return 600;
		case width >= 1280 && width < 1536:
			return 733;
		case width >= 1536 && width < 1792:
			return 866;
		case width >= 1792 && width < 2048:
			return 1000;
		case width >= 2048:
			return 1133;
	}
};
