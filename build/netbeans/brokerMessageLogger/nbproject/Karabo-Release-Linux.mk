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
OBJECTFILES= \
	${OBJECTDIR}/_ext/496226620/brokerMessageLogger.o


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
LDLIBSOPTIONS=-L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../lib -Wl,-rpath,\$$ORIGIN/../extern/lib -Wl,-rpath,${KARABO}/extern/lib -Wl,-rpath,../karabo/dist/Release/GNU-Linux-x86/lib -L../karabo/dist/Release/GNU-Linux-x86/lib -lkarabo -lboost_chrono -lboost_date_time -lboost_filesystem -lboost_python -lboost_regex -lboost_signals -lboost_system -lboost_thread -lcppunit -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp -llog4cpp -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lnetsnmpmibs -lnetsnmptrapd -lopenmqc -lpython2.7

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokermessagelogger

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokermessagelogger: ../karabo/dist/Release/GNU-Linux-x86/lib/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokermessagelogger: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokermessagelogger ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/496226620/brokerMessageLogger.o: ../../../src/brokerMessageLogger/brokerMessageLogger.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/496226620
	${RM} $@.d
	$(COMPILE.cc) -O2 -I${KARABO}/include -I${KARABO}/extern/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/496226620/brokerMessageLogger.o ../../../src/brokerMessageLogger/brokerMessageLogger.cc

# Subprojects
.build-subprojects:
	cd ../karabo && ${MAKE} -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bin/karabo-brokermessagelogger

# Subprojects
.clean-subprojects:
	cd ../karabo && ${MAKE} -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
