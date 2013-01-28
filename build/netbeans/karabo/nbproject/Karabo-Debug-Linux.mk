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
	${OBJECTDIR}/_ext/163016059/FromLiteral.o \
	${OBJECTDIR}/_ext/163016059/ClassInfo.o \
	${OBJECTDIR}/_ext/163016059/Exception.o \
	${OBJECTDIR}/_ext/163016059/FromTypeInfo.o \
	${OBJECTDIR}/_ext/163016059/StringTools.o \
	${OBJECTDIR}/_ext/163016059/Timer.o \
	${OBJECTDIR}/_ext/163016059/Base64.o \
	${OBJECTDIR}/_ext/163016059/Time.o \
	${OBJECTDIR}/_ext/163016059/Profiler.o \
	${OBJECTDIR}/_ext/163016059/Hash.o

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
LDLIBSOPTIONS=-L/opt/local/lib/nss -L/opt/local/lib/nspr -L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../extern/lib -lboost_chrono -lboost_date_time -lboost_filesystem -lboost_numpy -lboost_python -lboost_regex -lboost_signals -lboost_system -lboost_thread -lcppunit -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp -llog4cpp -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lnetsnmpmibs -lnetsnmptrapd -lopenmqc -lpython2.7 -ldl -lrt -lpthread -lX11

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${TESTDIR}/TestFiles/f5

${TESTDIR}/TestFiles/f5: ${OBJECTFILES}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -shared -o ${TESTDIR}/TestFiles/f5 -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/163016059/FromLiteral.o: ../../../src/karabo/util/FromLiteral.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/FromLiteral.o ../../../src/karabo/util/FromLiteral.cc

${OBJECTDIR}/_ext/163016059/ClassInfo.o: ../../../src/karabo/util/ClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/ClassInfo.o ../../../src/karabo/util/ClassInfo.cc

${OBJECTDIR}/_ext/163016059/Exception.o: ../../../src/karabo/util/Exception.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Exception.o ../../../src/karabo/util/Exception.cc

${OBJECTDIR}/_ext/163016059/FromTypeInfo.o: ../../../src/karabo/util/FromTypeInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/FromTypeInfo.o ../../../src/karabo/util/FromTypeInfo.cc

${OBJECTDIR}/_ext/163016059/StringTools.o: ../../../src/karabo/util/StringTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/StringTools.o ../../../src/karabo/util/StringTools.cc

${OBJECTDIR}/_ext/163016059/Timer.o: ../../../src/karabo/util/Timer.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Timer.o ../../../src/karabo/util/Timer.cc

${OBJECTDIR}/_ext/163016059/Base64.o: ../../../src/karabo/util/Base64.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Base64.o ../../../src/karabo/util/Base64.cc

${OBJECTDIR}/_ext/163016059/Time.o: ../../../src/karabo/util/Time.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Time.o ../../../src/karabo/util/Time.cc

${OBJECTDIR}/_ext/163016059/Profiler.o: ../../../src/karabo/util/Profiler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Profiler.o ../../../src/karabo/util/Profiler.cc

${OBJECTDIR}/_ext/163016059/Hash.o: ../../../src/karabo/util/Hash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	${RM} $@.d
	$(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Hash.o ../../../src/karabo/util/Hash.cc

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/_ext/1033104525/Configurator_Test.o ${TESTDIR}/_ext/1033104525/Factory_Test.o ${TESTDIR}/_ext/1033104525/Hash_Test.o ${TESTDIR}/_ext/1033104525/utilTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib 


${TESTDIR}/_ext/1033104525/Configurator_Test.o: ../../../src/karabo/tests/util/Configurator_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/1033104525
	${RM} $@.d
	$(COMPILE.cc) -g -DTESTPATH=\"${CND_BASEDIR}/../../../src/karabo/tests/\" -I${KARABO}/extern/include/hdf5 -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -MMD -MP -MF $@.d -o ${TESTDIR}/_ext/1033104525/Configurator_Test.o ../../../src/karabo/tests/util/Configurator_Test.cc


${TESTDIR}/_ext/1033104525/Factory_Test.o: ../../../src/karabo/tests/util/Factory_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/1033104525
	${RM} $@.d
	$(COMPILE.cc) -g -DTESTPATH=\"${CND_BASEDIR}/../../../src/karabo/tests/\" -I${KARABO}/extern/include/hdf5 -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -MMD -MP -MF $@.d -o ${TESTDIR}/_ext/1033104525/Factory_Test.o ../../../src/karabo/tests/util/Factory_Test.cc


${TESTDIR}/_ext/1033104525/Hash_Test.o: ../../../src/karabo/tests/util/Hash_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/1033104525
	${RM} $@.d
	$(COMPILE.cc) -g -DTESTPATH=\"${CND_BASEDIR}/../../../src/karabo/tests/\" -I${KARABO}/extern/include/hdf5 -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -MMD -MP -MF $@.d -o ${TESTDIR}/_ext/1033104525/Hash_Test.o ../../../src/karabo/tests/util/Hash_Test.cc


${TESTDIR}/_ext/1033104525/utilTestRunner.o: ../../../src/karabo/tests/util/utilTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/_ext/1033104525
	${RM} $@.d
	$(COMPILE.cc) -g -DTESTPATH=\"${CND_BASEDIR}/../../../src/karabo/tests/\" -I${KARABO}/extern/include/hdf5 -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -MMD -MP -MF $@.d -o ${TESTDIR}/_ext/1033104525/utilTestRunner.o ../../../src/karabo/tests/util/utilTestRunner.cc


${OBJECTDIR}/_ext/163016059/FromLiteral_nomain.o: ${OBJECTDIR}/_ext/163016059/FromLiteral.o ../../../src/karabo/util/FromLiteral.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/FromLiteral.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/FromLiteral_nomain.o ../../../src/karabo/util/FromLiteral.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/FromLiteral.o ${OBJECTDIR}/_ext/163016059/FromLiteral_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/ClassInfo_nomain.o: ${OBJECTDIR}/_ext/163016059/ClassInfo.o ../../../src/karabo/util/ClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/ClassInfo.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/ClassInfo_nomain.o ../../../src/karabo/util/ClassInfo.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/ClassInfo.o ${OBJECTDIR}/_ext/163016059/ClassInfo_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Exception_nomain.o: ${OBJECTDIR}/_ext/163016059/Exception.o ../../../src/karabo/util/Exception.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Exception.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Exception_nomain.o ../../../src/karabo/util/Exception.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Exception.o ${OBJECTDIR}/_ext/163016059/Exception_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/FromTypeInfo_nomain.o: ${OBJECTDIR}/_ext/163016059/FromTypeInfo.o ../../../src/karabo/util/FromTypeInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/FromTypeInfo.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/FromTypeInfo_nomain.o ../../../src/karabo/util/FromTypeInfo.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/FromTypeInfo.o ${OBJECTDIR}/_ext/163016059/FromTypeInfo_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/StringTools_nomain.o: ${OBJECTDIR}/_ext/163016059/StringTools.o ../../../src/karabo/util/StringTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/StringTools.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/StringTools_nomain.o ../../../src/karabo/util/StringTools.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/StringTools.o ${OBJECTDIR}/_ext/163016059/StringTools_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Timer_nomain.o: ${OBJECTDIR}/_ext/163016059/Timer.o ../../../src/karabo/util/Timer.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Timer.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Timer_nomain.o ../../../src/karabo/util/Timer.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Timer.o ${OBJECTDIR}/_ext/163016059/Timer_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Base64_nomain.o: ${OBJECTDIR}/_ext/163016059/Base64.o ../../../src/karabo/util/Base64.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Base64.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Base64_nomain.o ../../../src/karabo/util/Base64.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Base64.o ${OBJECTDIR}/_ext/163016059/Base64_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Time_nomain.o: ${OBJECTDIR}/_ext/163016059/Time.o ../../../src/karabo/util/Time.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Time.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Time_nomain.o ../../../src/karabo/util/Time.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Time.o ${OBJECTDIR}/_ext/163016059/Time_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Profiler_nomain.o: ${OBJECTDIR}/_ext/163016059/Profiler.o ../../../src/karabo/util/Profiler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Profiler.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Profiler_nomain.o ../../../src/karabo/util/Profiler.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Profiler.o ${OBJECTDIR}/_ext/163016059/Profiler_nomain.o;\
	fi

${OBJECTDIR}/_ext/163016059/Hash_nomain.o: ${OBJECTDIR}/_ext/163016059/Hash.o ../../../src/karabo/util/Hash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/163016059
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/163016059/Hash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} $@.d;\
	    $(COMPILE.cc) -g -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/python2.7 -fPIC  -Dmain=__nomain -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/163016059/Hash_nomain.o ../../../src/karabo/util/Hash.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/163016059/Hash.o ${OBJECTDIR}/_ext/163016059/Hash_nomain.o;\
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
	${RM} ${TESTDIR}/TestFiles/f5

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
