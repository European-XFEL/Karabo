project(
        "karabo"
        VERSION 2
        LANGUAGES CXX
)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All users of this library will need at least C++11

# finding dependencies
find_package(HDF5 REQUIRED)
find_package(OpenSSL REQUIRED)

find_library(OPENMQC openmqc)
find_library(LOG4CPP log4cpp)
find_library(SNAPPY snappy)

message(STATUS "CMAKE_INCLUDE_PATH = ${CMAKE_INCLUDE_PATH}")
message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENMQC = ${OPENMQC}")
message(STATUS "LOG4CPP = ${LOG4CPP}")
message(STATUS "SNAPPY = ${SNAPPY}")


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

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})

target_compile_options(${PROJECT_NAME} PRIVATE -Wfatal-errors -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wall -D__SO__)

include_directories(
        "${karabo_SOURCE_DIR}/.."
        "${KARABO_EXTERN_INCLUDE_DIR}"
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
        ${OPENMQC}
        ${SNAPPY}
        ${LOG4CPP}
        ${NSS}
        ${NSPR}
        ${PUGIXML}
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

enable_testing()

file(WRITE "${CMAKE_BINARY_DIR}/VERSION" ${KARABO_GIT_VERSION})

# Adding unit test
if (BUILD_TESTING)
    find_library(CPPUNIT cppunit)
    file(GLOB TEST_DIR "${karabo_SOURCE_DIR}/tests/*")
    foreach(TEST_DIR ${TEST_DIR})
        file(GLOB TEST_SRCS "${TEST_DIR}/*.cc")
        foreach (TMP_PATH ${TEST_SRCS})
            string (FIND ${TMP_PATH} "TestRunner" NAME_FOUND)
            if (NOT ${NAME_FOUND} EQUAL -1)
                string(REGEX REPLACE ".*/(.+)\.cc" "\\1" RUNNER_NAME "${TMP_PATH}")
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
