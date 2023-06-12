# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
function(find_dep LIBNAME INCLUDE_PATH)
    find_library(${LIBNAME}_LIB ${LIBNAME} PATHS $ENV{KARABO}/lib $ENV{KARABO}/extern/lib $ENV{CONDA_PREFIX}/lib)
    find_path(${LIBNAME}_INC_PATH ${INCLUDE_PATH} HINTS $ENV{KARABO}/include $ENV{KARABO}/extern/include $ENV{CONDA_PREFIX}/include)
    message(STATUS "${LIBNAME}_INC_PATH = ${${LIBNAME}_INC_PATH}")
    message(STATUS "${LIBNAME}_LIB = ${${LIBNAME}_LIB}")
endfunction()
