cmake_minimum_required(VERSION 3.12)
# TODO: lower the requirements for cmake
cmake_policy(SET CMP0074 NEW)

project("KaraboBundle"
    VERSION 2
    LANGUAGES C CXX
)

include( CTest )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
find_package(karabo_deps)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

add_subdirectory("src/karabo")
