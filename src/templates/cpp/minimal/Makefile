# Makefile to fool "karabo install" into building and installing the device
# using CMake.

CONF ?= Debug

# Keep default target as make all
all: install

# legacy target
build: install

# the .install.sh script will only build and copy into a dist directory
install:
	@./.install.sh ${CONF} $(patsubst -j%,%,$(filter -j%,$(MAKEFLAGS)))

clean:
	rm -fr dist
	rm -fr build

test: install
	cd build/__PACKAGE_NAME__; CTEST_OUTPUT_ON_FAILURE=1 ctest -vv
