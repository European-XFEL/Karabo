cmake_minimum_required(VERSION 3.15)

project(karabo-cpp-root-node)

if (BUILD_UNIT_TESTING OR BUILD_INTEGRATION_TESTING OR BUILD_LONG_RUN_TESTING)
  # If any test is being built, an enable_testing is needed at the root.
  # Otherwise the file CTestTestfile.cmake. required to guide ctest in
  # finding the tests will not be generated.
  enable_testing()
endif()

add_subdirectory(src)