const colorZone = (element, color) => {
	const text = color.substring(0, 6);
	const darkText = color.substring(6, 12);
	const background = color.substring(12, 18);
	const darkBackground = color.substring(18, 24);
	element.style.color = `light-dark(#${text},#${darkText})`;
	element.style.backgroundColor = `light-dark(#${background},#${darkBackground})`;
};
