CMAKE_BUILD_DIR ?= build
CMAKE = cmake
CTEST = ctest
BUILD_TYPE ?= Debug
NPROC ?= $(shell nproc)

.PHONY: all build configure compile test clean

all: build

configure:
	@mkdir -p $(CMAKE_BUILD_DIR)
	cd $(CMAKE_BUILD_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

build: configure
	cd $(CMAKE_BUILD_DIR) && $(CMAKE) --build . --parallel $(NPROC)

compile: build

test: build
	cd $(CMAKE_BUILD_DIR) && $(CTEST) --output-on-failure --parallel $(NPROC)

clean:
	rm -rf $(CMAKE_BUILD_DIR)
