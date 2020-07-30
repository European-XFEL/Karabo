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
CND_CONF=CodeCoverage
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/1625554341/brokerRates.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++14 -Wfatal-errors -fprofile-arcs -ftest-coverage
CXXFLAGS=-std=c++14 -Wfatal-errors -fprofile-arcs -ftest-coverage

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../lib -Wl,-rpath,\$$ORIGIN/../extern/lib -Wl,-rpath,${KARABO}/extern/lib -Wl,-rpath,../../karabo/dist/CodeCoverage/GNU-Linux-x86/lib -L../../karabo/dist/CodeCoverage/GNU-Linux-x86/lib -lkarabo `pkg-config --libs karaboDependencies-${CND_PLATFORM}`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokerrates

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokerrates: ../../karabo/dist/CodeCoverage/GNU-Linux-x86/lib/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokerrates: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokerrates ${OBJECTFILES} ${LDLIBSOPTIONS} -Wl,-rpath-link,${KARABO}/extern/lib

${OBJECTDIR}/_ext/1625554341/brokerRates.o: ../../../../src/tools/brokerRates/brokerRates.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1625554341
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/include -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/1625554341/brokerRates.o ../../../../src/tools/brokerRates/brokerRates.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokerrates

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
