cmake_minimum_required(VERSION 3.17)

project(
    "KarabindLib"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")
include("../cmake/resolve-karabo-lib-target.cmake")

resolveKaraboLibTarget()

set(KARABIND_LIB_TARGET_NAME "karabind")

# Uses Python3_ROOT_DIR to avoid other Python interpreters that maybe on the
# system with wrong NumPy version or no NumPy package at all.
set(Python3_ROOT_DIR ${CMAKE_PREFIX_PATH})
find_package(
    Python3 3.8 EXACT REQUIRED
    COMPONENTS Interpreter Development NumPy
)

message(STATUS "Found NumPy version '${Python3_NumPy_VERSION}'.")
message(STATUS "NumPy include dirs = '${Python3_NumPy_INCLUDE_DIRS}'")

# Here we are using 'pybind11' package instead of Boost Python
find_package(pybind11 CONFIG REQUIRED)

file(GLOB_RECURSE KARABIND_SRCS CONFIGURE_DEPENDS "*.cc")


#pybind11_add_module(karabind ${KARABIND_SRCS})

add_library(
    ${KARABIND_LIB_TARGET_NAME} SHARED
    ${KARABIND_SRCS}
)

# The last two RPATH entries are for when libkarabind.so is packaged as
# karabind.so inside INSTALL_PACKAGE_ROOT/extern/lib/python3.8/site-packages.
set_target_properties(
    ${KARABIND_LIB_TARGET_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../extern/lib;$ORIGIN;$ORIGIN/../../../../lib;$ORIGIN/../.."
    INTERPROCEDURAL_OPTIMIZATION ON
    CXX_VISIBILITY_PRESET default
)

# To avoid warnings about deprecated version for NumPy API.
target_compile_definitions(
    ${KARABIND_LIB_TARGET_NAME}
    PRIVATE
    NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION
)

# Options lifted from the legacy Netbeans based project.
target_compile_options(
    ${KARABIND_LIB_TARGET_NAME}
    PRIVATE
    -fvisibility=hidden
    -Wfatal-errors
    -Wno-unused-local-typedefs
    -Wno-noexcept-type
    -Wall)

target_include_directories(
    ${KARABIND_LIB_TARGET_NAME}
    PUBLIC
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
    # The Boost Python include directories are "inherited" from the
    # Karabo lib dependency - they are on the same tree as the other
    # components of Boost that libkarabo already used.
)

target_link_libraries(
    ${KARABIND_LIB_TARGET_NAME}
    ${KARABO_LIB_TARGET_NAME}
    pybind11::headers
    ${PYTHON_LIBRARIES}
)

install(
    TARGETS ${KARABIND_LIB_TARGET_NAME}
    LIBRARY DESTINATION "lib"
    PUBLIC_HEADER DESTINATION "include/karabind"
    RUNTIME DESTINATION "bin"
)

# Add karabind header files to the installation tree.
file(GLOB INCLUDE_FILES CONFIGURE_DEPENDS "*.hh")
foreach (FILE_PATH ${INCLUDE_FILES})
    install(FILES ${FILE_PATH} DESTINATION "include/karabind")
endforeach()