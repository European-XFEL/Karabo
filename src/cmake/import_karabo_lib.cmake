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
# Creates an IMPORTED target for the Karabo Framework Library by obtaining
# link libraries dependencies from a "karaboDependencies.pc" file expected
# at $KARABO/lib/karaboDependencies.pc.
#
# Also creates a variable, KARABO_LIB_TARGET_NAME, with the name of the
# IMPORTED target created for the library.
macro (importKaraboLib)

    if(NOT EXISTS "$ENV{KARABO}/lib/karaboDependencies.pc")
        message(FATAL_ERROR
                "'karaboDependencies.pc' file not found. Can't import Karabo Lib."
        )
    endif()

    set(KARABO_LIB_TARGET_NAME "karabo")

    find_library(
        KARABO_LIB ${KARABO_LIB_TARGET_NAME}
        PATHS $ENV{KARABO}/lib
    )

    # Retrieve the dependency libraries from $KARABO/lib/karaboDependencies.pc
    set(KARABO_LIB_LINK_LIBRARIES "")
    file(
        STRINGS
        "$ENV{KARABO}/lib/karaboDependencies.pc"
        LINES_READ
    )
    foreach(LINE ${LINES_READ})
        string(FIND ${LINE} "Libs: " LIBS_STR_POS)
        if(LIBS_STR_POS EQUAL 0)
            # Found the line of karaboDependencies with the dependency libs.
            # Extracts them.
            string(
                REGEX MATCHALL
                "-l[A-Za-z0-9]+[A-Za-z0-9_]*"
                KARABO_DEPS_LIBS
                ${LINE}
            )
            break()
        endif()
    endforeach()
    # Gathers the dependency libs that are available at $KARABO/extern/lib.
    # Assumes that other libs that are not in there, like libpthread, can
    # be found from the current CMAKE_PREFIX_PATH value.
    foreach(DEP_LIB ${KARABO_DEPS_LIBS})
        string(REPLACE "-l" "lib" DEP_LIB_SO ${DEP_LIB})
        string(APPEND DEP_LIB_SO ".so")
        if(EXISTS "$ENV{KARABO}/extern/lib/${DEP_LIB_SO}")
            list(
                APPEND
                KARABO_LIB_LINK_LIBRARIES
                "$ENV{KARABO}/extern/lib/${DEP_LIB_SO}"
            )
        endif()
    endforeach()
    list(LENGTH KARABO_LIB_LINK_LIBRARIES KARABO_LIB_DEPS_COUNT)
    if(${KARABO_LIB_DEPS_COUNT} EQUAL 0)
        message(
            FATAL_ERROR
            "No dependency of 'libkarabo' found at '$ENV{KARABO}/extern/lib'."
        )
    endif()

    add_library(${KARABO_LIB_TARGET_NAME} SHARED IMPORTED)

    set_target_properties(${KARABO_LIB_TARGET_NAME} PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "__SO__"
        INTERFACE_COMPILE_OPTIONS "-Wfatal-errors;-Wno-unused-local-typedefs;-Wno-deprecated-declarations;-Wall"
        INTERFACE_INCLUDE_DIRECTORIES "$ENV{KARABO}/include;$ENV{KARABO}/extern/include"
        INTERFACE_LINK_LIBRARIES "${KARABO_LIB_LINK_LIBRARIES}"
    )

    set_target_properties(${KARABO_LIB_TARGET_NAME} PROPERTIES
        IMPORTED_LOCATION ${KARABO_LIB}
    )

endmacro()
