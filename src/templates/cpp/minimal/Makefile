# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
# Makefile to fool "karabo install" into building and installing the device
# using CMake.

CONF ?= Debug

# Keep default target as make all
all: install

# legacy targets
build: install

package: install
	@# this script will look for a library file in dist/$(CONF)/cmake
	@$(KARABO)/bin/.bundle-cppplugin.sh dist $(CONF) cmake

# the .install.sh script will only build and copy into a dist directory
install:
	@./.install.sh ${CONF} $(patsubst -j%,%,$(filter -j%,$(MAKEFLAGS)))

clean:
	rm -fr dist
	rm -fr build

test: install
	@cd build && \
	cmake .. -DBUILD_TESTS=1 && \
	cmake --build . && \
	cd __PACKAGE_NAME__ && CTEST_OUTPUT_ON_FAILURE=1 ctest -VV
