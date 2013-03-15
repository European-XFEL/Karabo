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
	${OBJECTDIR}/_ext/684620707/HashWrap.o \
	${OBJECTDIR}/_ext/684620707/PyUtilHash.o \
	${OBJECTDIR}/_ext/684620707/karathon.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

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
LDLIBSOPTIONS=-L/opt/local/nss -L/opt/local/nspr -L${KARATHON}/karabo -L${KARATHON}/karabo/extern/lib -Wl,-rpath,\$$ORIGIN/karabo -Wl,-rpath,\$$ORIGIN/karabo/extern/lib -Wl,-rpath,/home/esenov/branch/karaboFramework/build/netbeans/karabo/dist/Debug/GNU-Linux-x86 -L/home/esenov/branch/karaboFramework/build/netbeans/karabo/dist/Debug/GNU-Linux-x86 -lkarabo -lboost_chrono -lboost_date_time -lboost_filesystem -lboost_numpy -lboost_python -lboost_regex -lboost_signals -lboost_system -lboost_thread -lcppunit -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp -llog4cpp -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lnetsnmpmibs -lnetsnmptrapd -lopenmqc -lpython2.7

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}: /home/esenov/branch/karaboFramework/build/netbeans/karabo/dist/Debug/GNU-Linux-x86/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/684620707/HashWrap.o: /home/esenov/branch/karaboFramework/src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	${RM} $@.d
	$(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/HashWrap.o /home/esenov/branch/karaboFramework/src/karathon/HashWrap.cc

${OBJECTDIR}/_ext/684620707/PyUtilHash.o: /home/esenov/branch/karaboFramework/src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	${RM} $@.d
	$(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/PyUtilHash.o /home/esenov/branch/karaboFramework/src/karathon/PyUtilHash.cc

${OBJECTDIR}/_ext/684620707/karathon.o: /home/esenov/branch/karaboFramework/src/karathon/karathon.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	${RM} $@.d
	$(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/karathon.o /home/esenov/branch/karaboFramework/src/karathon/karathon.cc

# Subprojects
.build-subprojects:
	cd /home/esenov/branch/karaboFramework/build/netbeans/karabo && ${MAKE}  -f Makefile CONF=Debug

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/HashWrap_Test.o ${TESTDIR}/tests/karathonTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} -Wl,-rpath,${KARATHON}/karabo/extern/lib -Wl,-rpath,${KARATHON}/karabo -Wl,-rpath,${KARATHON} `cppunit-config --libs`   


${TESTDIR}/tests/HashWrap_Test.o: tests/HashWrap_Test.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} $@.d
	$(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include `cppunit-config --cflags` -MMD -MP -MF $@.d -o ${TESTDIR}/tests/HashWrap_Test.o tests/HashWrap_Test.cc


${TESTDIR}/tests/karathonTestRunner.o: tests/karathonTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} $@.d
	$(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include `cppunit-config --cflags` -MMD -MP -MF $@.d -o ${TESTDIR}/tests/karathonTestRunner.o tests/karathonTestRunner.cc


${OBJECTDIR}/_ext/684620707/HashWrap_nomain.o: ${OBJECTDIR}/_ext/684620707/HashWrap.o /home/esenov/branch/karaboFramework/src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/684620707/HashWrap.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/HashWrap_nomain.o /home/esenov/branch/karaboFramework/src/karathon/HashWrap.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/684620707/HashWrap.o ${OBJECTDIR}/_ext/684620707/HashWrap_nomain.o;\
	fi

${OBJECTDIR}/_ext/684620707/PyUtilHash_nomain.o: ${OBJECTDIR}/_ext/684620707/PyUtilHash.o /home/esenov/branch/karaboFramework/src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/684620707/PyUtilHash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/PyUtilHash_nomain.o /home/esenov/branch/karaboFramework/src/karathon/PyUtilHash.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/684620707/PyUtilHash.o ${OBJECTDIR}/_ext/684620707/PyUtilHash_nomain.o;\
	fi

${OBJECTDIR}/_ext/684620707/karathon_nomain.o: ${OBJECTDIR}/_ext/684620707/karathon.o /home/esenov/branch/karaboFramework/src/karathon/karathon.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/684620707
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/684620707/karathon.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -DKARATHON_BOOST_NUMPY -I../../../src -I${KARATHON}/karabo/include -I${KARATHON}/karabo/extern/include/hdf5 -I${KARATHON}/karabo/extern/include/python2.7 -I${KARATHON}/karabo/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/684620707/karathon_nomain.o /home/esenov/branch/karaboFramework/src/karathon/karathon.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/684620707/karathon.o ${OBJECTDIR}/_ext/684620707/karathon_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd /home/esenov/branch/karaboFramework/build/netbeans/karabo && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
