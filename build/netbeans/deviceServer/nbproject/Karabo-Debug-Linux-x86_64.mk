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
LDLIBSOPTIONS=-L${EXTERN_DIST_DIR}/lib -Wl,-rpath,../lib -Wl,-rpath,../extern/lib -Wl,-rpath,${KARABO_DIST_DIR}/lib -Wl,-rpath,${EXTERN_DIST_DIR}/lib -Wl,-rpath,../karabo/dist/Debug/GNU-Linux-x86/lib -L../karabo/dist/Debug/GNU-Linux-x86/lib -lkarabo -lboost_date_time -lboost_filesystem -lboost_python -lboost_regex -lboost_signals -lboost_system -lboost_thread -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp -llog4cpp -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lnetsnmpmibs -lnetsnmptrapd -lopenmqc

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/karabo-deviceserver

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/karabo-deviceserver: ../karabo/dist/Debug/GNU-Linux-x86/lib/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/karabo-deviceserver: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/karabo-deviceserver ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/147234221/deviceServer.o: ../../../src/deviceServer/deviceServer.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/147234221
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${EXTERN_DIST_DIR}/include -I${KARABO_DIST_DIR}/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/147234221/deviceServer.o ../../../src/deviceServer/deviceServer.cc

# Subprojects
.build-subprojects:
	cd ../karabo && ${MAKE} -j12 -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/karabo-deviceserver

# Subprojects
.clean-subprojects:
	cd ../karabo && ${MAKE} -j12 -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
