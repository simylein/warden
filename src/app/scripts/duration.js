const duration = (seconds) => {
	switch (true) {
		case seconds < 1:
			return `${seconds.toFixed(1)}s`;
		case seconds >= 1 && seconds < 60:
			return `${Math.round(seconds)}s`;
		case seconds >= 60 && seconds < 600:
			return `${(seconds / 60).toFixed(1)}m`;
		case seconds >= 600 && seconds < 3600:
			return `${Math.round(seconds / 60)}m`;
		case seconds >= 3600 && seconds < 36000:
			return `${(seconds / 3600).toFixed(1)}h`;
		case seconds >= 36000:
			return `${Math.round(seconds / 3600)}h`;
	}
};
