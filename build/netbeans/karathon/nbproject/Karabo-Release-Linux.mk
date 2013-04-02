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
	${OBJECTDIR}/_ext/381567218/HashWrap.o \
	${OBJECTDIR}/_ext/381567218/PyIoFileTools.o \
	${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o \
	${OBJECTDIR}/_ext/381567218/PyUtilHash.o \
	${OBJECTDIR}/_ext/381567218/PyUtilSchema.o \
	${OBJECTDIR}/_ext/381567218/Wrapper.o \
	${OBJECTDIR}/_ext/381567218/karathon.o

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
LDLIBSOPTIONS=-L/opt/local/nss -L/opt/local/nspr -L${KARABO}/lib -L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN -Wl,-rpath,\$$ORIGIN/../extern/lib -Wl,-rpath,${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libkarathon.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/381567218/HashWrap.o: ../../../src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/HashWrap.o ../../../src/karathon/HashWrap.cc

${OBJECTDIR}/_ext/381567218/PyIoFileTools.o: ../../../src/karathon/PyIoFileTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ../../../src/karathon/PyIoFileTools.cc

${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o: ../../../src/karathon/PyUtilClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ../../../src/karathon/PyUtilClassInfo.cc

${OBJECTDIR}/_ext/381567218/PyUtilHash.o: ../../../src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilHash.o ../../../src/karathon/PyUtilHash.cc

${OBJECTDIR}/_ext/381567218/PyUtilSchema.o: ../../../src/karathon/PyUtilSchema.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o ../../../src/karathon/PyUtilSchema.cc

${OBJECTDIR}/_ext/381567218/Wrapper.o: ../../../src/karathon/Wrapper.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/Wrapper.o ../../../src/karathon/Wrapper.cc

${OBJECTDIR}/_ext/381567218/karathon.o: ../../../src/karathon/karathon.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/karathon.o ../../../src/karathon/karathon.cc

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/HashWrap_Test.o ${TESTDIR}/tests/karathonTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} `cppunit-config --libs`   


${TESTDIR}/tests/HashWrap_Test.o: tests/HashWrap_Test.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include `cppunit-config --cflags` -MMD -MP -MF $@.d -o ${TESTDIR}/tests/HashWrap_Test.o tests/HashWrap_Test.cc


${TESTDIR}/tests/karathonTestRunner.o: tests/karathonTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} $@.d
	$(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include `cppunit-config --cflags` -MMD -MP -MF $@.d -o ${TESTDIR}/tests/karathonTestRunner.o tests/karathonTestRunner.cc


${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o: ${OBJECTDIR}/_ext/381567218/HashWrap.o ../../../src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/HashWrap.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o ../../../src/karathon/HashWrap.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/HashWrap.o ${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o: ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ../../../src/karathon/PyIoFileTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o ../../../src/karathon/PyIoFileTools.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ../../../src/karathon/PyUtilClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o ../../../src/karathon/PyUtilClassInfo.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilHash_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilHash.o ../../../src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilHash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilHash_nomain.o ../../../src/karathon/PyUtilHash.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilHash.o ${OBJECTDIR}/_ext/381567218/PyUtilHash_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilSchema_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o ../../../src/karathon/PyUtilSchema.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/PyUtilSchema_nomain.o ../../../src/karathon/PyUtilSchema.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o ${OBJECTDIR}/_ext/381567218/PyUtilSchema_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/Wrapper_nomain.o: ${OBJECTDIR}/_ext/381567218/Wrapper.o ../../../src/karathon/Wrapper.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/Wrapper.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/Wrapper_nomain.o ../../../src/karathon/Wrapper.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/Wrapper.o ${OBJECTDIR}/_ext/381567218/Wrapper_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/karathon_nomain.o: ${OBJECTDIR}/_ext/381567218/karathon.o ../../../src/karathon/karathon.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/karathon.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -O2 -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python2.7 -I${KARABO}/extern/include -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/381567218/karathon_nomain.o ../../../src/karathon/karathon.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/karathon.o ${OBJECTDIR}/_ext/381567218/karathon_nomain.o;\
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

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
