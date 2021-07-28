#
# Tries  to infer the directory of a Karabo installation based on the
# behavior of "karabo new" and "karabo install" commands, which place the
# device source files at "$KARABO/devices".
#
# If the inference succeeds, KARABO env var is set to the infered directory and
# CMAKE_PREFIX_PATH is set to its "extern" subdirectory.
#
macro(inferKaraboEnv)
  string(FIND ${CMAKE_SOURCE_DIR} "/devices/" DEVICE_DIR_POS)
  if (DEVICE_DIR_POS GREATER 0
      AND EXISTS "${CMAKE_SOURCE_DIR}/../../bin/karabo-cppserver"
      AND EXISTS "${CMAKE_SOURCE_DIR}/../../activate")
        # "karabo-cppserver" and "activate" found at their expected locations
        # relative to infered Karabo directory. Set CMAKE_PREFIX_PATH so the
        # build dependencies can be found further in the project.
        get_filename_component(
            KARABO_INST
            "${CMAKE_SOURCE_DIR}/../.."
            ABSOLUTE
        )
        # sets the KARABO env var so it can be used downstream.
        set(ENV{KARABO} ${KARABO_INST})
        set(CMAKE_PREFIX_PATH
            ${CMAKE_PREFIX_PATH} ${KARABO_INST} ${KARABO_INST}/extern)
  endif()
endmacro()