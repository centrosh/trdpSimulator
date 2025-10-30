.PHONY: install lint test format check e2e

install:
	pip install --upgrade pip
	pip install -e .[dev]

lint:
	ruff check src tests

format:
	ruff format src tests
	ruff check --fix-only --exit-zero src tests

e2e:
	@echo "No end-to-end tests defined yet"

test:
	pytest

check: lint test
