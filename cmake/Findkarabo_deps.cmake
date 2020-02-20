find_package(Git REQUIRED)

# Check that we are using a valid OS

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    message(FATAL_ERROR "Karabo currently supports only Linux Operating systems")
endif()

# TODO: figure out how to use dependencies not shipped by this repository

# Check that we are building on a ci repository

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/.git")
    message(FATAL_ERROR "Not a git repository")
endif()

# fetch the latest deps tag

set(DEP_TAG_PATTERN "deps-*")
execute_process(COMMAND ${GIT_EXECUTABLE} tag -l ${DEP_TAG_PATTERN}
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAGS)
if("${GIT_TAGS}" STREQUAL "")
    message(FATAL_ERROR "No dependency tags found in this repository ${GIT_TAGS}")
endif()

# Step through commits in reverse chronological order
execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE REV_LIST)

STRING(REGEX REPLACE "\n" ";" GIT_REVS "${REV_LIST}")


function(fetch_deps DEP_TAG)
    execute_process(COMMAND lsb_release -is
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE OS_NAME_)
    string(STRIP "${OS_NAME_}" OS_NAME)

    execute_process(COMMAND lsb_release -rs
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE OS_VERSION)
    string(REGEX REPLACE "^([0-9]+).*" "\\1" OS_NUMBER "${OS_VERSION}")

    set(DEPS_OS_IDENTIFIER "${OS_NAME}-${OS_NUMBER}")

    set(DEPS_FILENAME "${DEPS_OS_IDENTIFIER}-${DEP_TAG}.tar.gz")
    message(STATUS "Fetching '${DEPS_FILENAME}'")

    if(NOT DEFINED DEP_URL_BASE)
        set(DEP_URL_BASE "http://exflserv05.desy.de/karabo/karaboDevelopmentDeps")
    endif()

    set(DEPS_URL "${DEP_URL_BASE}/${DEPS_FILENAME}")
    message(STATUS "Downloading binary file ${DEPS_URL}")
    execute_process(COMMAND curl -fO ${DEPS_URL}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    RESULT_VARIABLE CURL_RET)
    if(NOT ${CURL_RET} EQUAL 0)
        file(REMOVE ${DEPS_FILENAME})
        message(FATAL_ERROR "Fetching dependencies ${DEP_TAG} failed")
    endif()
    execute_process(COMMAND tar -zxf ${DEPS_FILENAME}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    message(STATUS "Unpacking Dependencies file ${DEPS_FILENAME}")
    file(REMOVE ${DEPS_FILENAME})
    SET(INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/extern")
    file(MAKE_DIRECTORY "${INSTALL_PREFIX}")
    file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${DEPS_OS_IDENTIFIER}" "${INSTALL_PREFIX}/${CMAKE_SYSTEM_NAME}")

    # refactoring stops here
    execute_process(COMMAND bash ${PROJECT_SOURCE_DIR}/extern/relocate_deps.sh "${INSTALL_PREFIX}/${CMAKE_SYSTEM_NAME}"
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/extern)
    set(MARKER_PATH "${CMAKE_CURRENT_BINARY_DIR}/extern/${CMAKE_SYSTEM_NAME}/.deps_tag.txt")
    file(WRITE "${MARKER_PATH}" "${DEP_TAG}")

endfunction()

function(fetch_deps_if_needed DEP_TAG)
    set(MARKER_PATH "${CMAKE_CURRENT_BINARY_DIR}/extern/${CMAKE_SYSTEM_NAME}/.deps_tag.txt")
    IF(EXISTS "${MARKER_PATH}")
        file(READ "${MARKER_PATH}" MARKER_)
        string(STRIP "${MARKER_}" INSTALLED_TAG)
        if (NOT "${INSTALLED_TAG}" STREQUAL "${DEP_TAG}")
            fetch_deps(${DEP_TAG})
        else()
            message(STATUS "Dependencies already installed")
        endif()
    else()
        fetch_deps(${DEP_TAG})
    endif()

endfunction()

foreach(REV ${GIT_REVS})
    execute_process(COMMAND ${GIT_EXECUTABLE}  tag -l ${DEP_TAG_PATTERN} --points-at ${REV}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE LTAGNAME_)
    string(STRIP "${LTAGNAME_}" LTAGNAME)
    if(NOT "${LTAGNAME}" STREQUAL "")
        fetch_deps_if_needed(${LTAGNAME})
        break()
    endif()
endforeach()

set(KARABO_EXTERN_DIR "${CMAKE_CURRENT_BINARY_DIR}/extern/${CMAKE_SYSTEM_NAME}")

set(HDF5_ROOT "${KARABO_EXTERN_DIR}")
find_package(HDF5 REQUIRED)

find_package(OpenSSL REQUIRED)
message(STATUS "OPENSSL_FOUND version ${OPENSSL_VERSION}")

set(BOOST_LIBRARYDIR "${KARABO_EXTERN_DIR}/lib")
set(BOOST_ROOT "${KARABO_EXTERN_DIR}")
set(Boost_NO_SYSTEM_PATHS ON)

find_package(Boost 1.66 REQUIRED 
             COMPONENTS chrono date_time filesystem regex signals system thread)
message(STATUS "Boost version ${Boost_VERSION}")

find_library(CPPUNIT cppunit  HINTS "${KARABO_EXTERN_DIR}/lib")
if(NOT CPPUNIT)
  message(FATAL_ERROR "CPPUNIT library not found")
endif()

find_library(OPENMQC openmqc HINTS "${KARABO_EXTERN_DIR}/lib")
if(NOT OPENMQC)
  message(FATAL_ERROR "OPENMQC library not found")
endif()

find_library(LOG4CPP log4cpp HINTS "${KARABO_EXTERN_DIR}/lib")
if(NOT LOG4CPP)
  message(FATAL_ERROR "LOG4CPP library not found")
endif()

find_library(SNAPPY snappy  HINTS "${KARABO_EXTERN_DIR}/lib")
if(NOT SNAPPY)
  message(FATAL_ERROR "SNAPPY library not found")
endif()

find_library(PUGIXML pugixml  HINTS "${KARABO_EXTERN_DIR}/lib")
if(NOT CPPUNIT)
  message(FATAL_ERROR "PUGIXML library not found")
endif()

set(KARABO_EXTERN_INCLUDE_DIR "${KARABO_EXTERN_DIR}/include")

execute_process(COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags HEAD
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE KARABO_GIT_VERSION)
