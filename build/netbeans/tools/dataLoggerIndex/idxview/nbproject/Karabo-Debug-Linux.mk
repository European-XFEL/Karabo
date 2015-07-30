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
	${OBJECTDIR}/_ext/1310162774/idxView.o


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
LDLIBSOPTIONS=-L${KARABO}/lib -L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../lib -Wl,-rpath,\$$ORIGIN/../extern/lib -lkarabo `pkg-config --libs karaboDependencies-${CND_PLATFORM}`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/idxview

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/idxview: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/idxview ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/1310162774/idxView.o: ../../../../../src/tools/dataLoggerIndex/idxView.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/1310162774
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../../../src/tools/dataLoggerIndex -I${KARABO}/include -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/1310162774/idxView.o ../../../../../src/tools/dataLoggerIndex/idxView.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/idxview

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
