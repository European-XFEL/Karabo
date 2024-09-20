cmake_minimum_required(VERSION 3.10)

project(daemontools-encore LANGUAGES C CXX)

include(CheckSymbolExists)
include(CheckIncludeFile)

check_symbol_exists(mkfifo "sys/stat.h" HASMKFIFO)
check_symbol_exists(flock "sys/file.h" HASFLOCK)
check_symbol_exists(sigprocmask "signal.h" HASSIGPROCMASK)
check_symbol_exists(sigaction "signal.h" HASSIGACTION)
check_symbol_exists(waitpid "sys/wait.h" HASWAITPID)

file(CONFIGURE OUTPUT hasmkffo.h CONTENT "#cmakedefine HASMKFIFO\n")
file(CONFIGURE OUTPUT hasflock.h CONTENT "#cmakedefine HASFLOCK\n")
file(CONFIGURE OUTPUT hassgprm.h CONTENT "#cmakedefine HASSIGPROCMASK\n")
file(CONFIGURE OUTPUT hassgact.h CONTENT "#cmakedefine HASSIGACTION\n")
file(CONFIGURE OUTPUT hasattribute.h CONTENT "#define HASATTRIBUTE\n")
file(CONFIGURE OUTPUT haswaitp.h CONTENT "#cmakedefine HASWAITPID\n")
file(CONFIGURE OUTPUT hasshsgr.h CONTENT "/* do not define HASSHORTSETGROUPS */\n")
file(CONFIGURE OUTPUT hasmemrchr.h CONTENT "#define HASMEMRCHR 1\n")

try_run(TRYULONG64_OUT DONTCARE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tryulong64.c)
if(${TRYULONG64_OUT})
    configure_file(uint64.h1 uint64.h)
else()
    configure_file(uint64.h2 uint64.h)
endif()

file(CONFIGURE OUTPUT conf_supervise.c CONTENT "const char conf_supervise[] = \"supervise\";\n")

try_run(TRYDRENT_OUT DONTCARE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/trydrent.c)
if(${TRYDRENT_OUT})
    configure_file(direntry.h1 direntry.h)
else()
    configure_file(direntry.h2 direntry.h)
endif()

try_run(TRYSYSEL_OUT DONTCARE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/trysysel.c)
if(${TRYSYSEL_OUT})
    configure_file(select.h1 select.h)
else()
    configure_file(select.h2 select.h)
endif()

try_run(TRYPOLL_OUT DONTCARE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/trypoll.c)
if(${TRYPOLL_OUT})
    configure_file(iopause.h1 iopause.h)
else()
    configure_file(iopause.h2 iopause.h)
endif()

file(READ warn-auto.sh SVSCANBOOT_1)
file(READ svscanboot.sh SVSCANBOOT_2)
file(CONFIGURE OUTPUT svscanboot CONTENT ${SVSCANBOOT_1}${SVSCANBOOT_2})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/svscanboot TYPE BIN)

add_compile_options(-Wall -Wshadow -Wcast-align -Wwrite-strings -Wno-implicit)

file(GLOB_RECURSE byte_srcs CONFIGURE_DEPENDS
    byte_*.c
    fmt_*.c
    scan_*.c
    str_*.c
)

file(GLOB_RECURSE unix_srcs CONFIGURE_DEPENDS
    alloc*.c
    buffer*.c
    closeonexec.c
    env.c
    error*.c
    fd*.c
    fifo.c
    lock*.c
    ndelay*.c
    open*.c
    pathexec*.c
    prot.c
    readclose.c
    seek_set.c
    sgetopt.c
    sig*.c
    stralloc*.c
    strerr*.c
    subgetopt.c
    wait*.c
)

file(GLOB_RECURSE time_srcs CONFIGURE_DEPENDS
    tai_*.c
    taia_*.c
    iopause.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_library(unix OBJECT ${unix_srcs})
add_library(byte OBJECT ${byte_srcs})
add_library(time OBJECT ${time_srcs})

add_executable(envdir envdir.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(envini envini.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(envuidgid envuidgid.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(fghack fghack.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
install(TARGETS envdir envini envuidgid fghack)

add_executable(multilog multilog.c deepsleep.c match.c timestamp.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte> $<TARGET_OBJECTS:time>)
add_executable(pgrphack pgrphack.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(readproctitle readproctitle.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
install(TARGETS multilog pgrphack readproctitle)

add_executable(setlock setlock.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(setuidgid setuidgid.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(setuser setuser.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(softlimit softlimit.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(supervise supervise.c svpath.c deepsleep.c subreaper.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte> $<TARGET_OBJECTS:time>)
install(TARGETS setlock setuidgid setuser softlimit supervise)

add_executable(svc svc.c svpath.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(svok svok.c svpath.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(svscan svscan.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
add_executable(svstat svstat.c svpath.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte> $<TARGET_OBJECTS:time>)
add_executable(svup svup.c svpath.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
install(TARGETS svc svok svscan svstat svup)

add_executable(tai64n tai64n.c timestamp.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte> $<TARGET_OBJECTS:time>)
add_executable(tai64nlocal tai64nlocal.c $<TARGET_OBJECTS:unix> $<TARGET_OBJECTS:byte>)
install(TARGETS tai64n tai64nlocal)
