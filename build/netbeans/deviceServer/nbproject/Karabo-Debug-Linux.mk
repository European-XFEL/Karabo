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
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/147234221/deviceServer.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../lib -Wl,-rpath,\$$ORIGIN/../extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -Wl,-rpath,../karabo/dist/Debug/GNU-Linux-x86/lib -L../karabo/dist/Debug/GNU-Linux-x86/lib -lkarabo `pkg-config --libs karaboDependencies-${CND_PLATFORM}`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-deviceserver

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-deviceserver: ../karabo/dist/Debug/GNU-Linux-x86/lib/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-deviceserver: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-deviceserver ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/147234221/deviceServer.o: ../../../src/deviceServer/deviceServer.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/147234221
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/include -I. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/147234221/deviceServer.o ../../../src/deviceServer/deviceServer.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-deviceserver

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
