cmake_minimum_required(VERSION 3.12)

project(
        "karabo"
        VERSION 2
        LANGUAGES C CXX
)
include(CTest)
cmake_policy(SET CMP0074 NEW)

# finding dependencies
set(EXTERN_DEPS_INCLUDE "" )
set(EXTERN_DEPS_LIBS "" )

find_package(HDF5 REQUIRED)
list(APPEND EXTERN_DEPS_LIBS ${HDF5_LIBRARIES})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


message(STATUS "Resolving GIT Version")
 
set(KARABO_VERSION "unknown")
 
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --match "*.*.*" --dirty --always 
    WORKING_DIRECTORY "${karabo_SOURCE_DIR}/../.."
    OUTPUT_VARIABLE KARABO_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message( STATUS "GIT VERSION: ${KARABO_VERSION}")
else()
  message(STATUS "GIT not found")
endif()
 
configure_file(${karabo_SOURCE_DIR}/util/repositoryVersion.in ${karabo_SOURCE_DIR}/util/repositoryVersion @ONLY)

# Karabo currently expect to find the hdf5 libraries in an `hdf5` folder
# here we trick the system by creating a link.
# TODO: fix this once the legacy dependencies are gone.
list(APPEND EXTERN_DEPS_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})
set(HDF5_SPOOF_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${HDF5_SPOOF_INCLUDE_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${HDF5_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/include/hdf5 )

list(APPEND EXTERN_DEPS_INCLUDE_DIRS ${HDF5_SPOOF_INCLUDE_DIR})

list(APPEND EXTERN_DEPS_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include/hdf5")

find_package(OpenSSL REQUIRED)
list(APPEND EXTERN_DEPS_LIBS ${OpenSSL_LIBRARIES})
list(APPEND EXTERN_DEPS_INCLUDE_DIRS ${OpenSSL_INCLUDE_DIRS})

find_package(Boost 1.66 REQUIRED
             COMPONENTS chrono date_time filesystem regex signals system thread)
list(APPEND EXTERN_DEPS_LIBS ${Boost_LIBRARIES})
list(APPEND EXTERN_DEPS_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})

function(add_dep LIBNAME INCLUDE_PATH)
    find_library(${LIBNAME}_LIB ${LIBNAME})
    find_path(${LIBNAME}_INC_PATH ${INCLUDE_PATH})
    list(APPEND EXTERN_DEPS_INCLUDE_DIRS ${${LIBNAME}_INC_PATH})
    list(APPEND EXTERN_DEPS_LIBS ${${LIBNAME}_LIB})
    message(STATUS "${LIBNAME}_INC_PATH = ${${LIBNAME}_INC_PATH}")
    message(STATUS "${LIBNAME}_LIB = ${${LIBNAME}_LIB}")
endfunction()

add_dep(log4cpp krb_log4cpp)
add_dep(pugixml pugixml.hpp)
add_dep(cppunit cppunit)
add_dep(openmqc openmqc)

list(REMOVE_DUPLICATES EXTERN_DEPS_INCLUDE_DIRS)

message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")

set(EXCLUDE_DIR "/tests/")

# collect the source files excluding the ones in the `tests` folder
file(GLOB_RECURSE SOURCE_FILES "*.cc")
# store test sources in TEST_SOURCE_FILES will be useful later
set(TEST_SOURCE_FILES "")
foreach (TMP_PATH ${SOURCE_FILES})
    string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM SOURCE_FILES ${TMP_PATH})
        list (APPEND TEST_SOURCE_FILES ${TMP_PATH})
    endif ()
endforeach()

# collect the header files excluding the ones in the `tests` folder
file(GLOB_RECURSE HDR_FILES "*.hh")
foreach (TMP_PATH ${HDR_FILES})
    string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM HDR_FILES ${TMP_PATH})
    endif ()
endforeach()

add_library(
        ${PROJECT_NAME} SHARED
        ${SOURCE_FILES}
)

target_compile_options(${PROJECT_NAME} PUBLIC -Wfatal-errors -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wall -D__SO__)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
include_directories(
    SYSTEM "${EXTERN_DEPS_INCLUDE_DIRS}"
)
include_directories(
    "${karabo_SOURCE_DIR}/.."    
)

source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADER_LIST})


# Adding flags to the compiler regarding the RPATH
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,${CMAKE_LIBRARY_PATH}")
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "${CMAKE_LIBRARY_PATH}" INSTALL_RPATH_USE_LINK_PATH TRUE)

# Adding dependecy libraries into the karabo project
target_link_libraries(
        ${PROJECT_NAME}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_CHRONO_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${Boost_SIGNALS_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${HDF5_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${openmqc_LIB}
        ${log4cpp_LIB}
        ${pugixml_LIB}
)

# Headers to be installed
file(GLOB INCLUDE_FILES "*.hpp")

# append all header files. TODO: implement a cleaner interface
foreach (TMP_PATH ${HDR_FILES})
   list (APPEND INCLUDE_FILES ${TMP_PATH})
endforeach()

# Creating karabo folder in include and lib folders
install(DIRECTORY DESTINATION "lib")
install(DIRECTORY DESTINATION "include/${PROJECT_NAME}")

# adding headers with the same tree directory of the project
foreach (file ${INCLUDE_FILES})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION "include/${PROJECT_NAME}/${dir}")
endforeach()

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION "lib"
        PUBLIC_HEADER DESTINATION "include/${PROJECT_NAME}"
        RUNTIME DESTINATION bin)

file(WRITE "${CMAKE_BINARY_DIR}/VERSION" ${KARABO_GIT_VERSION})
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/testresults")

# Adding unit test
if (BUILD_TESTING)
    find_library(CPPUNIT cppunit)
    file(GLOB TEST_DIR "${karabo_SOURCE_DIR}/tests/*")
    foreach(TEST_DIR ${TEST_DIR})
        file(GLOB TEST_SRCS "${TEST_DIR}/*.cc")
        foreach (TMP_PATH ${TEST_SRCS})
            string (FIND ${TMP_PATH} "TestRunner" NAME_FOUND)
            if (NOT ${NAME_FOUND} EQUAL -1)
                GET_FILENAME_COMPONENT(RUNNER_NAME ${TMP_PATH} NAME_WE)
                break()
            endif ()
        endforeach()
        ADD_EXECUTABLE("${RUNNER_NAME}" ${TEST_SRCS})
        target_compile_options("${RUNNER_NAME}" PRIVATE -DKARABO_TESTPATH=\"${karabo_SOURCE_DIR}/tests/\")
        target_compile_features("${RUNNER_NAME}" PUBLIC cxx_std_11)
        target_link_libraries(
                ${RUNNER_NAME}
                ${PROJECT_NAME}
                ${CPPUNIT}
        )
        ADD_TEST(NAME ${RUNNER_NAME} COMMAND ${RUNNER_NAME})
        set_tests_properties(${RUNNER_NAME} PROPERTIES
            ENVIRONMENT KARABO=${CMAKE_BINARY_DIR})
    endforeach()
endif()
