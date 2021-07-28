# Makefile to fool "karabo install" into building and installing the device
# using CMake.

CONF ?= Debug

# Keep default target as make all
all: install

install:
	@./.install.sh ${CONF}

clean:
	rm -fr dist
	rm -fr build

test: install
	cd build
	ctest
