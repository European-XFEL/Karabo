# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required(VERSION 3.15)

project(karabo-cpp-root-node)

if (BUILD_UNIT_TESTING OR BUILD_INTEGRATION_TESTING OR BUILD_LONG_RUN_TESTING)
  # If any test is being built, an enable_testing is needed at the root.
  # Otherwise the file CTestTestfile.cmake. required to guide ctest in
  # finding the tests will not be generated.
  enable_testing()
endif()

add_subdirectory(src)
