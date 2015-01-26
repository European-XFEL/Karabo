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
	${OBJECTDIR}/_ext/381567218/ChannelWrap.o \
	${OBJECTDIR}/_ext/381567218/ConnectionWrap.o \
	${OBJECTDIR}/_ext/381567218/FromNumpy.o \
	${OBJECTDIR}/_ext/381567218/HashWrap.o \
	${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o \
	${OBJECTDIR}/_ext/381567218/PyH5Tools.o \
	${OBJECTDIR}/_ext/381567218/PyIoFileTools.o \
	${OBJECTDIR}/_ext/381567218/PyLogLogger.o \
	${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o \
	${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o \
	${OBJECTDIR}/_ext/381567218/PyUtilDims.o \
	${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o \
	${OBJECTDIR}/_ext/381567218/PyUtilHash.o \
	${OBJECTDIR}/_ext/381567218/PyUtilSchema.o \
	${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o \
	${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o \
	${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o \
	${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o \
	${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o \
	${OBJECTDIR}/_ext/381567218/PyXipImage.o \
	${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o \
	${OBJECTDIR}/_ext/381567218/PyXipStatistics.o \
	${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o \
	${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o \
	${OBJECTDIR}/_ext/381567218/PythonInputHandler.o \
	${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o \
	${OBJECTDIR}/_ext/381567218/Wrapper.o \
	${OBJECTDIR}/_ext/381567218/karathon.o \
	${OBJECTDIR}/_ext/381567218/p2pbinding.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wno-unused-local-typedefs
CXXFLAGS=-Wno-unused-local-typedefs

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/lib -L${KARABO}/extern/lib -Wl,-rpath,\$$ORIGIN/../../../../../lib -Wl,-rpath,\$$ORIGIN/../../.. -Wl,-rpath,../karabo/dist/Release/GNU-Linux-x86/lib -L../karabo/dist/Release/GNU-Linux-x86/lib -lkarabo `pkg-config --libs karathonDependencies-${CND_PLATFORM}`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib/karathon.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib/karathon.${CND_DLIB_EXT}: ../karabo/dist/Release/GNU-Linux-x86/lib/libkarabo.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib/karathon.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib/karathon.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/381567218/ChannelWrap.o: ../../../src/karathon/ChannelWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/ChannelWrap.o ../../../src/karathon/ChannelWrap.cc

${OBJECTDIR}/_ext/381567218/ConnectionWrap.o: ../../../src/karathon/ConnectionWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/ConnectionWrap.o ../../../src/karathon/ConnectionWrap.cc

${OBJECTDIR}/_ext/381567218/FromNumpy.o: ../../../src/karathon/FromNumpy.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/FromNumpy.o ../../../src/karathon/FromNumpy.cc

${OBJECTDIR}/_ext/381567218/HashWrap.o: ../../../src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/HashWrap.o ../../../src/karathon/HashWrap.cc

${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o: ../../../src/karathon/PyCoreDeviceClient.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o ../../../src/karathon/PyCoreDeviceClient.cc

${OBJECTDIR}/_ext/381567218/PyH5Tools.o: ../../../src/karathon/PyH5Tools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyH5Tools.o ../../../src/karathon/PyH5Tools.cc

${OBJECTDIR}/_ext/381567218/PyIoFileTools.o: ../../../src/karathon/PyIoFileTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ../../../src/karathon/PyIoFileTools.cc

${OBJECTDIR}/_ext/381567218/PyLogLogger.o: ../../../src/karathon/PyLogLogger.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyLogLogger.o ../../../src/karathon/PyLogLogger.cc

${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o: ../../../src/karathon/PyUtilClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ../../../src/karathon/PyUtilClassInfo.cc

${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o: ../../../src/karathon/PyUtilDateTimeString.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o ../../../src/karathon/PyUtilDateTimeString.cc

${OBJECTDIR}/_ext/381567218/PyUtilDims.o: ../../../src/karathon/PyUtilDims.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilDims.o ../../../src/karathon/PyUtilDims.cc

${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o: ../../../src/karathon/PyUtilEpochstamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o ../../../src/karathon/PyUtilEpochstamp.cc

${OBJECTDIR}/_ext/381567218/PyUtilHash.o: ../../../src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilHash.o ../../../src/karathon/PyUtilHash.cc

${OBJECTDIR}/_ext/381567218/PyUtilSchema.o: ../../../src/karathon/PyUtilSchema.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o ../../../src/karathon/PyUtilSchema.cc

${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o: ../../../src/karathon/PyUtilTimeDuration.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o ../../../src/karathon/PyUtilTimeDuration.cc

${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o: ../../../src/karathon/PyUtilTimestamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o ../../../src/karathon/PyUtilTimestamp.cc

${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o: ../../../src/karathon/PyUtilTrainstamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o ../../../src/karathon/PyUtilTrainstamp.cc

${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o: ../../../src/karathon/PyWebAuthenticator.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o ../../../src/karathon/PyWebAuthenticator.cc

${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o: ../../../src/karathon/PyXipCpuImage.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`  -Wno-unused-but-set-variable -Wno-strict-aliasing -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o ../../../src/karathon/PyXipCpuImage.cc

${OBJECTDIR}/_ext/381567218/PyXipImage.o: ../../../src/karathon/PyXipImage.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipImage.o ../../../src/karathon/PyXipImage.cc

${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o: ../../../src/karathon/PyXipRawImageData.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o ../../../src/karathon/PyXipRawImageData.cc

${OBJECTDIR}/_ext/381567218/PyXipStatistics.o: ../../../src/karathon/PyXipStatistics.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipStatistics.o ../../../src/karathon/PyXipStatistics.cc

${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o: ../../../src/karathon/PyXmsSignalSlotable.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o ../../../src/karathon/PyXmsSignalSlotable.cc

${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o: ../../../src/karathon/PyXmsSlotElement.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o ../../../src/karathon/PyXmsSlotElement.cc

${OBJECTDIR}/_ext/381567218/PythonInputHandler.o: ../../../src/karathon/PythonInputHandler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PythonInputHandler.o ../../../src/karathon/PythonInputHandler.cc

${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o: ../../../src/karathon/PythonOutputHandler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o ../../../src/karathon/PythonOutputHandler.cc

${OBJECTDIR}/_ext/381567218/Wrapper.o: ../../../src/karathon/Wrapper.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/Wrapper.o ../../../src/karathon/Wrapper.cc

${OBJECTDIR}/_ext/381567218/karathon.o: ../../../src/karathon/karathon.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/karathon.o ../../../src/karathon/karathon.cc

${OBJECTDIR}/_ext/381567218/p2pbinding.o: ../../../src/karathon/p2pbinding.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/p2pbinding.o ../../../src/karathon/p2pbinding.cc

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-conf ${TESTFILES}
${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/HashWrap_Test.o ${TESTDIR}/tests/karathonTestRunner.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} -L${KARABO}/extern/lib -Wl,-rpath,${KARATHON}/karabo/extern/lib -Wl,-rpath,${KARATHON}/karabo -Wl,-rpath,${KARATHON} `cppunit-config --libs`   


${TESTDIR}/tests/HashWrap_Test.o: tests/HashWrap_Test.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`  `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/HashWrap_Test.o tests/HashWrap_Test.cc


${TESTDIR}/tests/karathonTestRunner.o: tests/karathonTestRunner.cc 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`  `cppunit-config --cflags` -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/karathonTestRunner.o tests/karathonTestRunner.cc


${OBJECTDIR}/_ext/381567218/ChannelWrap_nomain.o: ${OBJECTDIR}/_ext/381567218/ChannelWrap.o ../../../src/karathon/ChannelWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/ChannelWrap.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/ChannelWrap_nomain.o ../../../src/karathon/ChannelWrap.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/ChannelWrap.o ${OBJECTDIR}/_ext/381567218/ChannelWrap_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/ConnectionWrap_nomain.o: ${OBJECTDIR}/_ext/381567218/ConnectionWrap.o ../../../src/karathon/ConnectionWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/ConnectionWrap.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/ConnectionWrap_nomain.o ../../../src/karathon/ConnectionWrap.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/ConnectionWrap.o ${OBJECTDIR}/_ext/381567218/ConnectionWrap_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/FromNumpy_nomain.o: ${OBJECTDIR}/_ext/381567218/FromNumpy.o ../../../src/karathon/FromNumpy.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/FromNumpy.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/FromNumpy_nomain.o ../../../src/karathon/FromNumpy.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/FromNumpy.o ${OBJECTDIR}/_ext/381567218/FromNumpy_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o: ${OBJECTDIR}/_ext/381567218/HashWrap.o ../../../src/karathon/HashWrap.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/HashWrap.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o ../../../src/karathon/HashWrap.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/HashWrap.o ${OBJECTDIR}/_ext/381567218/HashWrap_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient_nomain.o: ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o ../../../src/karathon/PyCoreDeviceClient.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient_nomain.o ../../../src/karathon/PyCoreDeviceClient.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient.o ${OBJECTDIR}/_ext/381567218/PyCoreDeviceClient_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyH5Tools_nomain.o: ${OBJECTDIR}/_ext/381567218/PyH5Tools.o ../../../src/karathon/PyH5Tools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyH5Tools.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyH5Tools_nomain.o ../../../src/karathon/PyH5Tools.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyH5Tools.o ${OBJECTDIR}/_ext/381567218/PyH5Tools_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o: ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ../../../src/karathon/PyIoFileTools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o ../../../src/karathon/PyIoFileTools.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyIoFileTools.o ${OBJECTDIR}/_ext/381567218/PyIoFileTools_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyLogLogger_nomain.o: ${OBJECTDIR}/_ext/381567218/PyLogLogger.o ../../../src/karathon/PyLogLogger.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyLogLogger.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyLogLogger_nomain.o ../../../src/karathon/PyLogLogger.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyLogLogger.o ${OBJECTDIR}/_ext/381567218/PyLogLogger_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ../../../src/karathon/PyUtilClassInfo.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o ../../../src/karathon/PyUtilClassInfo.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo.o ${OBJECTDIR}/_ext/381567218/PyUtilClassInfo_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o ../../../src/karathon/PyUtilDateTimeString.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString_nomain.o ../../../src/karathon/PyUtilDateTimeString.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString.o ${OBJECTDIR}/_ext/381567218/PyUtilDateTimeString_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilDims_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilDims.o ../../../src/karathon/PyUtilDims.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilDims.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilDims_nomain.o ../../../src/karathon/PyUtilDims.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilDims.o ${OBJECTDIR}/_ext/381567218/PyUtilDims_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o ../../../src/karathon/PyUtilEpochstamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp_nomain.o ../../../src/karathon/PyUtilEpochstamp.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp.o ${OBJECTDIR}/_ext/381567218/PyUtilEpochstamp_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilHash_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilHash.o ../../../src/karathon/PyUtilHash.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilHash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilHash_nomain.o ../../../src/karathon/PyUtilHash.cc;\
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
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilSchema_nomain.o ../../../src/karathon/PyUtilSchema.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilSchema.o ${OBJECTDIR}/_ext/381567218/PyUtilSchema_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o ../../../src/karathon/PyUtilTimeDuration.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration_nomain.o ../../../src/karathon/PyUtilTimeDuration.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration.o ${OBJECTDIR}/_ext/381567218/PyUtilTimeDuration_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilTimestamp_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o ../../../src/karathon/PyUtilTimestamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp_nomain.o ../../../src/karathon/PyUtilTimestamp.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp.o ${OBJECTDIR}/_ext/381567218/PyUtilTimestamp_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp_nomain.o: ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o ../../../src/karathon/PyUtilTrainstamp.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp_nomain.o ../../../src/karathon/PyUtilTrainstamp.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp.o ${OBJECTDIR}/_ext/381567218/PyUtilTrainstamp_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyWebAuthenticator_nomain.o: ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o ../../../src/karathon/PyWebAuthenticator.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator_nomain.o ../../../src/karathon/PyWebAuthenticator.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator.o ${OBJECTDIR}/_ext/381567218/PyWebAuthenticator_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXipCpuImage_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o ../../../src/karathon/PyXipCpuImage.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`  -Wno-unused-but-set-variable -Wno-strict-aliasing -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipCpuImage_nomain.o ../../../src/karathon/PyXipCpuImage.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXipCpuImage.o ${OBJECTDIR}/_ext/381567218/PyXipCpuImage_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXipImage_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXipImage.o ../../../src/karathon/PyXipImage.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXipImage.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipImage_nomain.o ../../../src/karathon/PyXipImage.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXipImage.o ${OBJECTDIR}/_ext/381567218/PyXipImage_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXipRawImageData_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o ../../../src/karathon/PyXipRawImageData.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipRawImageData_nomain.o ../../../src/karathon/PyXipRawImageData.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXipRawImageData.o ${OBJECTDIR}/_ext/381567218/PyXipRawImageData_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXipStatistics_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXipStatistics.o ../../../src/karathon/PyXipStatistics.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXipStatistics.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXipStatistics_nomain.o ../../../src/karathon/PyXipStatistics.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXipStatistics.o ${OBJECTDIR}/_ext/381567218/PyXipStatistics_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o ../../../src/karathon/PyXmsSignalSlotable.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable_nomain.o ../../../src/karathon/PyXmsSignalSlotable.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable.o ${OBJECTDIR}/_ext/381567218/PyXmsSignalSlotable_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PyXmsSlotElement_nomain.o: ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o ../../../src/karathon/PyXmsSlotElement.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement_nomain.o ../../../src/karathon/PyXmsSlotElement.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement.o ${OBJECTDIR}/_ext/381567218/PyXmsSlotElement_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PythonInputHandler_nomain.o: ${OBJECTDIR}/_ext/381567218/PythonInputHandler.o ../../../src/karathon/PythonInputHandler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PythonInputHandler.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PythonInputHandler_nomain.o ../../../src/karathon/PythonInputHandler.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PythonInputHandler.o ${OBJECTDIR}/_ext/381567218/PythonInputHandler_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/PythonOutputHandler_nomain.o: ${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o ../../../src/karathon/PythonOutputHandler.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/PythonOutputHandler_nomain.o ../../../src/karathon/PythonOutputHandler.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/PythonOutputHandler.o ${OBJECTDIR}/_ext/381567218/PythonOutputHandler_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/Wrapper_nomain.o: ${OBJECTDIR}/_ext/381567218/Wrapper.o ../../../src/karathon/Wrapper.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/Wrapper.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/Wrapper_nomain.o ../../../src/karathon/Wrapper.cc;\
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
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/karathon_nomain.o ../../../src/karathon/karathon.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/karathon.o ${OBJECTDIR}/_ext/381567218/karathon_nomain.o;\
	fi

${OBJECTDIR}/_ext/381567218/p2pbinding_nomain.o: ${OBJECTDIR}/_ext/381567218/p2pbinding.o ../../../src/karathon/p2pbinding.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/381567218
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/381567218/p2pbinding.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Wall -I../../../src -I${KARABO}/include -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/python3.4 -I${KARABO}/extern/lib/python3.4/site-packages/numpy/core/include -I${KARABO}/extern/include `pkg-config --cflags karathonDependencies-${CND_PLATFORM}`   -fPIC  -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/381567218/p2pbinding_nomain.o ../../../src/karathon/p2pbinding.cc;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/381567218/p2pbinding.o ${OBJECTDIR}/_ext/381567218/p2pbinding_nomain.o;\
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
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/lib/karathon.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
