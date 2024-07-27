refresh: clean build install lint

build:
	python -m build	--wheel --sdist

install: 
	pip install .	

install_dev:
	pip install -e .

build_dist:
	make clean
	python -m build
	pip install dist/*.whl
	make test

release:
	python -m twine upload dist/*

lint:
	flake8 constellation/ tests/ --count --max-line-length=127 --ignore=W503
	mypy constellation/ --follow-imports=skip

test:
	python -m unittest

clean:
	rm -rf __pycache__
	rm -rf tests/__pycache__
	find constellation/ -name "__pycache__" -type d -exec rm -rf "{}" +
	find constellation/ -name "*.egg-info" -type d -exec rm -rf "{}" +
	rm -rf build
	rm -rf dist
	rm -rf "constellation.egg-info"
	rm -rf .pytest_cache
	rm -rf .mypy_cache
	pip uninstall -y constellation || true