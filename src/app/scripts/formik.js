const formik = (elements, schema, submit) => {
	const [values, errors, touched] = [{}, {}, {}];

	const validate = (all) => {
		if (all) Object.keys(schema).forEach((key) => (touched[key] = true));
		Object.keys(touched).forEach((key) => {
			if (touched[key]) errors[key] = schema[key](values[key]);
		});
		Object.keys(errors).forEach((key) => {
			Object.keys(elements)
				.filter((element) => element === key)
				.forEach((element) => {
					if (errors[key]) {
						elements[element].innerText = errors[key];
						elements[element].classList.remove('hidden');
					} else {
						elements[element].innerText = '';
						elements[element].classList.add('hidden');
					}
				});
		});
		const valid = Object.keys(errors).every((key) => !errors[key]);
		if (valid) {
			elements.button.disabled = false;
			elements.button.classList.add('background-blue-300', 'dark:background-blue-700', 'cursor-pointer');
			elements.button.classList.remove('background-neutral-300', 'dark:background-neutral-700', 'cursor-not-allowed');
		} else if (all) {
			elements.button.disabled = true;
			elements.button.classList.add('background-neutral-300', 'dark:background-neutral-700', 'cursor-not-allowed');
			elements.button.classList.remove('background-blue-300', 'dark:background-blue-700', 'cursor-pointer');
		}
		return valid;
	};

	const setValue = (key, value) => {
		values[key] = value;
		validate();
	};

	const setTouched = (key, value) => {
		touched[key] = value;
		validate();
	};

	const handleSubmit = async (event) => {
		event.preventDefault();
		if (!validate(true)) return;
		const store = elements.button.innerText;
		elements.button.innerText = 'loading...';
		elements.button.disabled = true;
		elements.button.classList.add('cursor-progress');
		elements.button.classList.remove('cursor-pointer');
		await submit(values);
		elements.button.innerText = store;
		elements.button.disabled = false;
		elements.button.classList.add('cursor-pointer');
		elements.button.classList.remove('cursor-progress');
	};

	return { values, errors, touched, validate, setValue, setTouched, handleSubmit };
};
