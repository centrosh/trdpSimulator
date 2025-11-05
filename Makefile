.PHONY: configure build test clean

BUILD_DIR ?= build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=RelWithDebInfo

build: configure
	cmake --build $(BUILD_DIR)

test: configure
	cmake --build $(BUILD_DIR)
	ctest --test-dir $(BUILD_DIR) --output-on-failure
	python3 -m pytest

clean:
	rm -rf $(BUILD_DIR)
