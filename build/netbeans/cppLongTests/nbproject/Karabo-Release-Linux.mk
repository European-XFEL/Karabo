#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES=

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1 \
	${TESTDIR}/TestFiles/f2

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wno-unused-local-typedefs -std=c++14
CXXFLAGS=-Wno-unused-local-typedefs -std=c++14

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/extern/lib -L${KARABO}/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lkarabo `pkg-config --libs karaboDependencies-${CND_PLATFORM}` -lcppunit  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcppLongTests.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcppLongTests.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcppLongTests.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -Wl,-rpath-link,${KARABO}/extern/lib -shared -fPIC

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/_ext/565560950/BaseLogging_Test.o ${TESTDIR}/_ext/565560950/TelegrafLogging_Test.o ${TESTDIR}/_ext/565560950/deviceTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} `cppunit-config --libs`   

${TESTDIR}/TestFiles/f2: ${TESTDIR}/_ext/783893685/InputOutputChannel_LongTest.o ${TESTDIR}/_ext/783893685/SignalSlotable_LongTest.o ${TESTDIR}/_ext/783893685/xmsLongTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f2 $^ ${LDLIBSOPTIONS} 


${TESTDIR}/_ext/565560950/BaseLogging_Test.o: ../../../src/cppLongTests/devices/BaseLogging_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/565560950
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/565560950/BaseLogging_Test.o ../../../src/cppLongTests/devices/BaseLogging_Test.cc


${TESTDIR}/_ext/565560950/TelegrafLogging_Test.o: ../../../src/cppLongTests/devices/TelegrafLogging_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/565560950
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/565560950/TelegrafLogging_Test.o ../../../src/cppLongTests/devices/TelegrafLogging_Test.cc


${TESTDIR}/_ext/565560950/deviceTestRunner.o: ../../../src/cppLongTests/devices/deviceTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/_ext/565560950
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/565560950/deviceTestRunner.o ../../../src/cppLongTests/devices/deviceTestRunner.cc


${TESTDIR}/_ext/783893685/InputOutputChannel_LongTest.o: ../../../src/cppLongTests/xms/InputOutputChannel_LongTest.cc 
	${MKDIR} -p ${TESTDIR}/_ext/783893685
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/783893685/InputOutputChannel_LongTest.o ../../../src/cppLongTests/xms/InputOutputChannel_LongTest.cc


${TESTDIR}/_ext/783893685/SignalSlotable_LongTest.o: ../../../src/cppLongTests/xms/SignalSlotable_LongTest.cc 
	${MKDIR} -p ${TESTDIR}/_ext/783893685
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/783893685/SignalSlotable_LongTest.o ../../../src/cppLongTests/xms/SignalSlotable_LongTest.cc


${TESTDIR}/_ext/783893685/xmsLongTestRunner.o: ../../../src/cppLongTests/xms/xmsLongTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/_ext/783893685
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -I../../../src -I${KARABO}/extern/include -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/783893685/xmsLongTestRunner.o ../../../src/cppLongTests/xms/xmsLongTestRunner.cc


# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	    ${TESTDIR}/TestFiles/f2 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libcppLongTests.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
