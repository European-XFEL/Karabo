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
	${TESTDIR}/TestFiles/alarmservice_test \
	${TESTDIR}/TestFiles/f10 \
	${TESTDIR}/TestFiles/f9 \
	${TESTDIR}/TestFiles/devicelocking_test \
	${TESTDIR}/TestFiles/f6 \
	${TESTDIR}/TestFiles/pipelinedprocessing_test \
	${TESTDIR}/TestFiles/property_test \
	${TESTDIR}/TestFiles/runtimeschemaattributes_test \
	${TESTDIR}/TestFiles/f7 \
	${TESTDIR}/TestFiles/f8

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wfatal-errors -Wno-unused-local-typedefs -std=c++14
CXXFLAGS=-Wfatal-errors -Wno-unused-local-typedefs -std=c++14

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/extern/lib -L${KARABO}/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lkarabo `pkg-config --libs karaboDependencies-${CND_PLATFORM}`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libintegrationTests.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libintegrationTests.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libintegrationTests.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -Wl,-rpath-link,${KARABO}/extern/lib -shared -fPIC

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/alarmservice_test: ${TESTDIR}/_ext/567603001/AlarmService_Test.o ${TESTDIR}/_ext/567603001/AlarmTesterDevice.o ${TESTDIR}/_ext/567603001/TcpAdapter.o ${TESTDIR}/_ext/567603001/integrationRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/alarmservice_test $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/f10: ${TESTDIR}/_ext/567603001/DataLogging_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_11.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f10 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/f9: ${TESTDIR}/_ext/567603001/Device_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_9.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f9 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit 

${TESTDIR}/TestFiles/devicelocking_test: ${TESTDIR}/_ext/567603001/LockTestDevice.o ${TESTDIR}/_ext/567603001/LockTest_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_5.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/devicelocking_test $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/f6: ${TESTDIR}/_ext/567603001/GuiServer_Test.o ${TESTDIR}/_ext/567603001/TcpAdapter_5.o ${TESTDIR}/_ext/567603001/integrationRunner_10.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f6 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit 

${TESTDIR}/TestFiles/pipelinedprocessing_test: ${TESTDIR}/_ext/567603001/P2PSenderDevice.o ${TESTDIR}/_ext/567603001/PipeReceiverDevice.o ${TESTDIR}/_ext/567603001/PipelinedProcessing_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_4.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/pipelinedprocessing_test $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/property_test: ${TESTDIR}/_ext/567603001/PropertyTest_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_2.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/property_test $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/runtimeschemaattributes_test: ${TESTDIR}/_ext/567603001/AlarmTesterDevice_3.o ${TESTDIR}/_ext/567603001/RunTimeSchemaAttributes_Test.o ${TESTDIR}/_ext/567603001/TcpAdapter_3.o ${TESTDIR}/_ext/567603001/integrationRunner_3.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/runtimeschemaattributes_test $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit `pkg-config cppunit --libs`   

${TESTDIR}/TestFiles/f7: ${TESTDIR}/_ext/567603001/NonSceneProviderTestDevice.o ${TESTDIR}/_ext/567603001/SceneProviderTestDevice.o ${TESTDIR}/_ext/567603001/SceneProvider_Test.o ${TESTDIR}/_ext/567603001/TcpAdapter_4.o ${TESTDIR}/_ext/567603001/integrationRunner_7.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f7 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit 

${TESTDIR}/TestFiles/f8: ${TESTDIR}/_ext/567603001/SimulatedTimeServerDevice.o ${TESTDIR}/_ext/567603001/TimingTestDevice.o ${TESTDIR}/_ext/567603001/Timing_Test.o ${TESTDIR}/_ext/567603001/integrationRunner_8.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f8 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib -lcppunit 


${TESTDIR}/_ext/567603001/AlarmService_Test.o: ../../../src/integrationTests/AlarmService_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/AlarmService_Test.o ../../../src/integrationTests/AlarmService_Test.cc


${TESTDIR}/_ext/567603001/AlarmTesterDevice.o: ../../../src/integrationTests/AlarmTesterDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/AlarmTesterDevice.o ../../../src/integrationTests/AlarmTesterDevice.cc


${TESTDIR}/_ext/567603001/TcpAdapter.o: ../../../src/integrationTests/TcpAdapter.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/TcpAdapter.o ../../../src/integrationTests/TcpAdapter.cc


${TESTDIR}/_ext/567603001/integrationRunner.o: ../../../src/integrationTests/integrationRunner.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner.o ../../../src/integrationTests/integrationRunner.cc


${TESTDIR}/_ext/567603001/DataLogging_Test.o: ../../../src/integrationTests/DataLogging_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/DataLogging_Test.o ../../../src/integrationTests/DataLogging_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_11.o: ../../../src/integrationTests/integrationRunner_11.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_11.o ../../../src/integrationTests/integrationRunner_11.cc


${TESTDIR}/_ext/567603001/Device_Test.o: ../../../src/integrationTests/Device_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/Device_Test.o ../../../src/integrationTests/Device_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_9.o: ../../../src/integrationTests/integrationRunner_9.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_9.o ../../../src/integrationTests/integrationRunner_9.cc


${TESTDIR}/_ext/567603001/LockTestDevice.o: ../../../src/integrationTests/LockTestDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/LockTestDevice.o ../../../src/integrationTests/LockTestDevice.cc


${TESTDIR}/_ext/567603001/LockTest_Test.o: ../../../src/integrationTests/LockTest_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/LockTest_Test.o ../../../src/integrationTests/LockTest_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_5.o: ../../../src/integrationTests/integrationRunner_5.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_5.o ../../../src/integrationTests/integrationRunner_5.cc


${TESTDIR}/_ext/567603001/GuiServer_Test.o: ../../../src/integrationTests/GuiServer_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/GuiServer_Test.o ../../../src/integrationTests/GuiServer_Test.cc


${TESTDIR}/_ext/567603001/TcpAdapter_5.o: ../../../src/integrationTests/TcpAdapter_5.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/TcpAdapter_5.o ../../../src/integrationTests/TcpAdapter_5.cc


${TESTDIR}/_ext/567603001/integrationRunner_10.o: ../../../src/integrationTests/integrationRunner_10.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_10.o ../../../src/integrationTests/integrationRunner_10.cc


${TESTDIR}/_ext/567603001/P2PSenderDevice.o: ../../../src/integrationTests/P2PSenderDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/P2PSenderDevice.o ../../../src/integrationTests/P2PSenderDevice.cc


${TESTDIR}/_ext/567603001/PipeReceiverDevice.o: ../../../src/integrationTests/PipeReceiverDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/PipeReceiverDevice.o ../../../src/integrationTests/PipeReceiverDevice.cc


${TESTDIR}/_ext/567603001/PipelinedProcessing_Test.o: ../../../src/integrationTests/PipelinedProcessing_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/PipelinedProcessing_Test.o ../../../src/integrationTests/PipelinedProcessing_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_4.o: ../../../src/integrationTests/integrationRunner_4.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_4.o ../../../src/integrationTests/integrationRunner_4.cc


${TESTDIR}/_ext/567603001/PropertyTest_Test.o: ../../../src/integrationTests/PropertyTest_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/PropertyTest_Test.o ../../../src/integrationTests/PropertyTest_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_2.o: ../../../src/integrationTests/integrationRunner_2.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_2.o ../../../src/integrationTests/integrationRunner_2.cc


${TESTDIR}/_ext/567603001/AlarmTesterDevice_3.o: ../../../src/integrationTests/AlarmTesterDevice_3.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/AlarmTesterDevice_3.o ../../../src/integrationTests/AlarmTesterDevice_3.cc


${TESTDIR}/_ext/567603001/RunTimeSchemaAttributes_Test.o: ../../../src/integrationTests/RunTimeSchemaAttributes_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/RunTimeSchemaAttributes_Test.o ../../../src/integrationTests/RunTimeSchemaAttributes_Test.cc


${TESTDIR}/_ext/567603001/TcpAdapter_3.o: ../../../src/integrationTests/TcpAdapter_3.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/TcpAdapter_3.o ../../../src/integrationTests/TcpAdapter_3.cc


${TESTDIR}/_ext/567603001/integrationRunner_3.o: ../../../src/integrationTests/integrationRunner_3.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`  `pkg-config cppunit --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_3.o ../../../src/integrationTests/integrationRunner_3.cc


${TESTDIR}/_ext/567603001/NonSceneProviderTestDevice.o: ../../../src/integrationTests/NonSceneProviderTestDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/NonSceneProviderTestDevice.o ../../../src/integrationTests/NonSceneProviderTestDevice.cc


${TESTDIR}/_ext/567603001/SceneProviderTestDevice.o: ../../../src/integrationTests/SceneProviderTestDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/SceneProviderTestDevice.o ../../../src/integrationTests/SceneProviderTestDevice.cc


${TESTDIR}/_ext/567603001/SceneProvider_Test.o: ../../../src/integrationTests/SceneProvider_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/SceneProvider_Test.o ../../../src/integrationTests/SceneProvider_Test.cc


${TESTDIR}/_ext/567603001/TcpAdapter_4.o: ../../../src/integrationTests/TcpAdapter_4.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/TcpAdapter_4.o ../../../src/integrationTests/TcpAdapter_4.cc


${TESTDIR}/_ext/567603001/integrationRunner_7.o: ../../../src/integrationTests/integrationRunner_7.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_7.o ../../../src/integrationTests/integrationRunner_7.cc


${TESTDIR}/_ext/567603001/SimulatedTimeServerDevice.o: ../../../src/integrationTests/SimulatedTimeServerDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/SimulatedTimeServerDevice.o ../../../src/integrationTests/SimulatedTimeServerDevice.cc


${TESTDIR}/_ext/567603001/TimingTestDevice.o: ../../../src/integrationTests/TimingTestDevice.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/TimingTestDevice.o ../../../src/integrationTests/TimingTestDevice.cc


${TESTDIR}/_ext/567603001/Timing_Test.o: ../../../src/integrationTests/Timing_Test.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/Timing_Test.o ../../../src/integrationTests/Timing_Test.cc


${TESTDIR}/_ext/567603001/integrationRunner_8.o: ../../../src/integrationTests/integrationRunner_8.cc 
	${MKDIR} -p ${TESTDIR}/_ext/567603001
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -D__SO__ -DKARABO_TESTPATH=\"${CND_BASEDIR}/../../../src/integrationTests/\" -I../../../src -I${KARABO}/extern/include -I${KARABO}/extern/include/hdf5 -I. `pkg-config --cflags karaboDependencies-${CND_PLATFORM}`   -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/567603001/integrationRunner_8.o ../../../src/integrationTests/integrationRunner_8.cc


# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/alarmservice_test || true; \
	    ${TESTDIR}/TestFiles/f10 || true; \
	    ${TESTDIR}/TestFiles/f9 || true; \
	    ${TESTDIR}/TestFiles/devicelocking_test || true; \
	    ${TESTDIR}/TestFiles/f6 || true; \
	    ${TESTDIR}/TestFiles/pipelinedprocessing_test || true; \
	    ${TESTDIR}/TestFiles/property_test || true; \
	    ${TESTDIR}/TestFiles/runtimeschemaattributes_test || true; \
	    ${TESTDIR}/TestFiles/f7 || true; \
	    ${TESTDIR}/TestFiles/f8 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libintegrationTests.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
