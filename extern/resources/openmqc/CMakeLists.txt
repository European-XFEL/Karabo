cmake_minimum_required(VERSION 3.10)

project(openmqc LANGUAGES C CXX)

find_package(Threads REQUIRED)
find_package(nss REQUIRED)

add_library(${CMAKE_PROJECT_NAME} SHARED)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp PRTypesUtils.c)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS cclient/cshim/*.h)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
    ${sources})

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES PUBLIC_HEADER
    "${headers}")

target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
    Threads::Threads
    nss::nss
)

install(TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION "lib"
    PUBLIC_HEADER DESTINATION "include/openmqc")
