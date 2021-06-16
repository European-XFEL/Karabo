cmake_minimum_required(VERSION 3.12)

project(
    "KarathonLib"
    LANGUAGES C CXX
)

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")
include("../../cmake/resolve-karabo-lib-target.cmake")

resolveKaraboLibTarget()

set(KARATHON_LIB_TARGET_NAME "karathon")

# NumPy is explicitly mentioned to avoid using an incompatible version
# already installed in the system.
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter Development NumPy)
message(STATUS "Found NumPy version '${Python3_NumPy_VERSION}'.")
message(STATUS "NumPy include dirs = '${Python3_NumPy_INCLUDE_DIRS}'")

# From Boost 1.67 onwards, the identifier of the Boost Python component
# requires a suffix with the version of Python the Boost Python package has
# been built against. More details at:
# https://stackoverflow.com/questions/56067518/cmake-could-not-find-boost-python
find_package(Boost 1.68 REQUIRED COMPONENTS python38)

file(GLOB_RECURSE KARATHON_SRCS "*.cc")

add_library(
    ${KARATHON_LIB_TARGET_NAME} SHARED
    ${KARATHON_SRCS}
)

# To avoid warnings about deprecated version for NumPy API.
target_compile_definitions(
    ${KARATHON_LIB_TARGET_NAME}
    PRIVATE
    NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION
)

# Options lifted from the legacy Netbeans based project.
target_compile_options(
    ${KARATHON_LIB_TARGET_NAME}
    PRIVATE
    -Wfatal-errors
    -Wno-unused-local-typedefs
    -Wno-noexcept-type
    -Wall)

target_include_directories(
    ${KARATHON_LIB_TARGET_NAME}
    PUBLIC
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
    # The Boost Python include directories are "inherited" from the
    # Karabo lib dependency - they are on the same tree as the other
    # components of Boost that libkarabo already used.
)

target_link_libraries(
    ${KARATHON_LIB_TARGET_NAME}
    ${KARABO_LIB_TARGET_NAME}
    Boost::python38
    ${PYTHON_LIBRARIES}
)

install(
    TARGETS ${KARATHON_LIB_TARGET_NAME}
    LIBRARY DESTINATION "lib"
    PUBLIC_HEADER DESTINATION "include/karathon"
    RUNTIME DESTINATION "bin"
)
