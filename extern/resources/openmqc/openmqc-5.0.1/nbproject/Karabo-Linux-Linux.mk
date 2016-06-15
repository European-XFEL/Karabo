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
CND_CONF=Linux
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/basictypes/BasicType.o \
	${OBJECTDIR}/src/basictypes/Boolean.o \
	${OBJECTDIR}/src/basictypes/Byte.o \
	${OBJECTDIR}/src/basictypes/Double.o \
	${OBJECTDIR}/src/basictypes/Float.o \
	${OBJECTDIR}/src/basictypes/HandledObject.o \
	${OBJECTDIR}/src/basictypes/Integer.o \
	${OBJECTDIR}/src/basictypes/Long.o \
	${OBJECTDIR}/src/basictypes/Monitor.o \
	${OBJECTDIR}/src/basictypes/Object.o \
	${OBJECTDIR}/src/basictypes/Short.o \
	${OBJECTDIR}/src/basictypes/UTF8String.o \
	${OBJECTDIR}/src/client/BytesMessage.o \
	${OBJECTDIR}/src/client/Connection.o \
	${OBJECTDIR}/src/client/Destination.o \
	${OBJECTDIR}/src/client/FlowControl.o \
	${OBJECTDIR}/src/client/Message.o \
	${OBJECTDIR}/src/client/MessageConsumer.o \
	${OBJECTDIR}/src/client/MessageConsumerTable.o \
	${OBJECTDIR}/src/client/MessageID.o \
	${OBJECTDIR}/src/client/MessageProducer.o \
	${OBJECTDIR}/src/client/NSSInitCall.o \
	${OBJECTDIR}/src/client/PingTimer.o \
	${OBJECTDIR}/src/client/PortMapperClient.o \
	${OBJECTDIR}/src/client/ProducerFlow.o \
	${OBJECTDIR}/src/client/ProtocolHandler.o \
	${OBJECTDIR}/src/client/ReadChannel.o \
	${OBJECTDIR}/src/client/ReadQTable.o \
	${OBJECTDIR}/src/client/ReceiveQueue.o \
	${OBJECTDIR}/src/client/Session.o \
	${OBJECTDIR}/src/client/SessionMutex.o \
	${OBJECTDIR}/src/client/SessionQueueReader.o \
	${OBJECTDIR}/src/client/TextMessage.o \
	${OBJECTDIR}/src/client/XASession.o \
	${OBJECTDIR}/src/client/XIDObject.o \
	${OBJECTDIR}/src/client/auth/JMQBasicAuthenticationHandler.o \
	${OBJECTDIR}/src/client/auth/JMQDigestAuthenticationHandler.o \
	${OBJECTDIR}/src/client/protocol/SSLProtocolHandler.o \
	${OBJECTDIR}/src/client/protocol/StubProtocolHandler.o \
	${OBJECTDIR}/src/client/protocol/TCPProtocolHandler.o \
	${OBJECTDIR}/src/containers/BasicTypeHashtable.o \
	${OBJECTDIR}/src/containers/ObjectVector.o \
	${OBJECTDIR}/src/containers/Properties.o \
	${OBJECTDIR}/src/containers/StringKeyHashtable.o \
	${OBJECTDIR}/src/containers/Vector.o \
	${OBJECTDIR}/src/cshim/iMQBytesMessageShim.o \
	${OBJECTDIR}/src/cshim/iMQCallbacks.o \
	${OBJECTDIR}/src/cshim/iMQConnectionShim.o \
	${OBJECTDIR}/src/cshim/iMQConsumerShim.o \
	${OBJECTDIR}/src/cshim/iMQDestinationShim.o \
	${OBJECTDIR}/src/cshim/iMQLogUtilsShim.o \
	${OBJECTDIR}/src/cshim/iMQMessageShim.o \
	${OBJECTDIR}/src/cshim/iMQProducerShim.o \
	${OBJECTDIR}/src/cshim/iMQPropertiesShim.o \
	${OBJECTDIR}/src/cshim/iMQSSLShim.o \
	${OBJECTDIR}/src/cshim/iMQSessionShim.o \
	${OBJECTDIR}/src/cshim/iMQStatusShim.o \
	${OBJECTDIR}/src/cshim/iMQTextMessageShim.o \
	${OBJECTDIR}/src/cshim/iMQTypes.o \
	${OBJECTDIR}/src/cshim/shimUtils.o \
	${OBJECTDIR}/src/cshim/xaswitch.o \
	${OBJECTDIR}/src/error/ErrorCodes.o \
	${OBJECTDIR}/src/error/ErrorTrace.o \
	${OBJECTDIR}/src/io/IMQDataInputStream.o \
	${OBJECTDIR}/src/io/IMQDataOutputStream.o \
	${OBJECTDIR}/src/io/Packet.o \
	${OBJECTDIR}/src/io/PacketFlag.o \
	${OBJECTDIR}/src/io/PacketProperties.o \
	${OBJECTDIR}/src/io/PacketType.o \
	${OBJECTDIR}/src/io/PortMapperEntry.o \
	${OBJECTDIR}/src/io/PortMapperTable.o \
	${OBJECTDIR}/src/io/SSLSocket.o \
	${OBJECTDIR}/src/io/SocketTest.o \
	${OBJECTDIR}/src/io/Status.o \
	${OBJECTDIR}/src/io/SysMessageID.o \
	${OBJECTDIR}/src/io/TCPSocket.o \
	${OBJECTDIR}/src/net/IPAddress.o \
	${OBJECTDIR}/src/serial/SerialDataInputStream.o \
	${OBJECTDIR}/src/serial/SerialDataOutputStream.o \
	${OBJECTDIR}/src/serial/SerialHandleManager.o \
	${OBJECTDIR}/src/serial/Serialize.o \
	${OBJECTDIR}/src/util/Logger.o \
	${OBJECTDIR}/src/util/MemAllocTest.o \
	${OBJECTDIR}/src/util/PRTypesUtils.o \
	${OBJECTDIR}/src/util/utf8.o


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
LDLIBSOPTIONS=-L${EXTERN_DISTDIR}/lib -Wl,-rpath,\$$ORIGIN -lssl3 -lsmime3 -lnssutil3 -lnss3 -lplds4 -lplc4 -lnspr4

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk lib/libopenmqc.${CND_DLIB_EXT}

lib/libopenmqc.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p lib
	g++ -o lib/libopenmqc.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/src/basictypes/BasicType.o: src/basictypes/BasicType.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/BasicType.o src/basictypes/BasicType.cpp

${OBJECTDIR}/src/basictypes/Boolean.o: src/basictypes/Boolean.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Boolean.o src/basictypes/Boolean.cpp

${OBJECTDIR}/src/basictypes/Byte.o: src/basictypes/Byte.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Byte.o src/basictypes/Byte.cpp

${OBJECTDIR}/src/basictypes/Double.o: src/basictypes/Double.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Double.o src/basictypes/Double.cpp

${OBJECTDIR}/src/basictypes/Float.o: src/basictypes/Float.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Float.o src/basictypes/Float.cpp

${OBJECTDIR}/src/basictypes/HandledObject.o: src/basictypes/HandledObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/HandledObject.o src/basictypes/HandledObject.cpp

${OBJECTDIR}/src/basictypes/Integer.o: src/basictypes/Integer.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Integer.o src/basictypes/Integer.cpp

${OBJECTDIR}/src/basictypes/Long.o: src/basictypes/Long.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Long.o src/basictypes/Long.cpp

${OBJECTDIR}/src/basictypes/Monitor.o: src/basictypes/Monitor.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Monitor.o src/basictypes/Monitor.cpp

${OBJECTDIR}/src/basictypes/Object.o: src/basictypes/Object.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Object.o src/basictypes/Object.cpp

${OBJECTDIR}/src/basictypes/Short.o: src/basictypes/Short.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/Short.o src/basictypes/Short.cpp

${OBJECTDIR}/src/basictypes/UTF8String.o: src/basictypes/UTF8String.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/basictypes
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/basictypes/UTF8String.o src/basictypes/UTF8String.cpp

${OBJECTDIR}/src/client/BytesMessage.o: src/client/BytesMessage.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/BytesMessage.o src/client/BytesMessage.cpp

${OBJECTDIR}/src/client/Connection.o: src/client/Connection.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/Connection.o src/client/Connection.cpp

${OBJECTDIR}/src/client/Destination.o: src/client/Destination.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/Destination.o src/client/Destination.cpp

${OBJECTDIR}/src/client/FlowControl.o: src/client/FlowControl.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/FlowControl.o src/client/FlowControl.cpp

${OBJECTDIR}/src/client/Message.o: src/client/Message.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/Message.o src/client/Message.cpp

${OBJECTDIR}/src/client/MessageConsumer.o: src/client/MessageConsumer.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/MessageConsumer.o src/client/MessageConsumer.cpp

${OBJECTDIR}/src/client/MessageConsumerTable.o: src/client/MessageConsumerTable.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/MessageConsumerTable.o src/client/MessageConsumerTable.cpp

${OBJECTDIR}/src/client/MessageID.o: src/client/MessageID.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/MessageID.o src/client/MessageID.cpp

${OBJECTDIR}/src/client/MessageProducer.o: src/client/MessageProducer.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/MessageProducer.o src/client/MessageProducer.cpp

${OBJECTDIR}/src/client/NSSInitCall.o: src/client/NSSInitCall.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/NSSInitCall.o src/client/NSSInitCall.cpp

${OBJECTDIR}/src/client/PingTimer.o: src/client/PingTimer.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/PingTimer.o src/client/PingTimer.cpp

${OBJECTDIR}/src/client/PortMapperClient.o: src/client/PortMapperClient.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/PortMapperClient.o src/client/PortMapperClient.cpp

${OBJECTDIR}/src/client/ProducerFlow.o: src/client/ProducerFlow.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/ProducerFlow.o src/client/ProducerFlow.cpp

${OBJECTDIR}/src/client/ProtocolHandler.o: src/client/ProtocolHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/ProtocolHandler.o src/client/ProtocolHandler.cpp

${OBJECTDIR}/src/client/ReadChannel.o: src/client/ReadChannel.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/ReadChannel.o src/client/ReadChannel.cpp

${OBJECTDIR}/src/client/ReadQTable.o: src/client/ReadQTable.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/ReadQTable.o src/client/ReadQTable.cpp

${OBJECTDIR}/src/client/ReceiveQueue.o: src/client/ReceiveQueue.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/ReceiveQueue.o src/client/ReceiveQueue.cpp

${OBJECTDIR}/src/client/Session.o: src/client/Session.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/Session.o src/client/Session.cpp

${OBJECTDIR}/src/client/SessionMutex.o: src/client/SessionMutex.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/SessionMutex.o src/client/SessionMutex.cpp

${OBJECTDIR}/src/client/SessionQueueReader.o: src/client/SessionQueueReader.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/SessionQueueReader.o src/client/SessionQueueReader.cpp

${OBJECTDIR}/src/client/TextMessage.o: src/client/TextMessage.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/TextMessage.o src/client/TextMessage.cpp

${OBJECTDIR}/src/client/XASession.o: src/client/XASession.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/XASession.o src/client/XASession.cpp

${OBJECTDIR}/src/client/XIDObject.o: src/client/XIDObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/XIDObject.o src/client/XIDObject.cpp

${OBJECTDIR}/src/client/auth/JMQBasicAuthenticationHandler.o: src/client/auth/JMQBasicAuthenticationHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client/auth
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/auth/JMQBasicAuthenticationHandler.o src/client/auth/JMQBasicAuthenticationHandler.cpp

${OBJECTDIR}/src/client/auth/JMQDigestAuthenticationHandler.o: src/client/auth/JMQDigestAuthenticationHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client/auth
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/auth/JMQDigestAuthenticationHandler.o src/client/auth/JMQDigestAuthenticationHandler.cpp

${OBJECTDIR}/src/client/protocol/SSLProtocolHandler.o: src/client/protocol/SSLProtocolHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client/protocol
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/protocol/SSLProtocolHandler.o src/client/protocol/SSLProtocolHandler.cpp

${OBJECTDIR}/src/client/protocol/StubProtocolHandler.o: src/client/protocol/StubProtocolHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client/protocol
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/protocol/StubProtocolHandler.o src/client/protocol/StubProtocolHandler.cpp

${OBJECTDIR}/src/client/protocol/TCPProtocolHandler.o: src/client/protocol/TCPProtocolHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/client/protocol
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/client/protocol/TCPProtocolHandler.o src/client/protocol/TCPProtocolHandler.cpp

${OBJECTDIR}/src/containers/BasicTypeHashtable.o: src/containers/BasicTypeHashtable.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/containers
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containers/BasicTypeHashtable.o src/containers/BasicTypeHashtable.cpp

${OBJECTDIR}/src/containers/ObjectVector.o: src/containers/ObjectVector.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/containers
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containers/ObjectVector.o src/containers/ObjectVector.cpp

${OBJECTDIR}/src/containers/Properties.o: src/containers/Properties.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/containers
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containers/Properties.o src/containers/Properties.cpp

${OBJECTDIR}/src/containers/StringKeyHashtable.o: src/containers/StringKeyHashtable.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/containers
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containers/StringKeyHashtable.o src/containers/StringKeyHashtable.cpp

${OBJECTDIR}/src/containers/Vector.o: src/containers/Vector.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/containers
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containers/Vector.o src/containers/Vector.cpp

${OBJECTDIR}/src/cshim/iMQBytesMessageShim.o: src/cshim/iMQBytesMessageShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQBytesMessageShim.o src/cshim/iMQBytesMessageShim.cpp

${OBJECTDIR}/src/cshim/iMQCallbacks.o: src/cshim/iMQCallbacks.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQCallbacks.o src/cshim/iMQCallbacks.cpp

${OBJECTDIR}/src/cshim/iMQConnectionShim.o: src/cshim/iMQConnectionShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQConnectionShim.o src/cshim/iMQConnectionShim.cpp

${OBJECTDIR}/src/cshim/iMQConsumerShim.o: src/cshim/iMQConsumerShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQConsumerShim.o src/cshim/iMQConsumerShim.cpp

${OBJECTDIR}/src/cshim/iMQDestinationShim.o: src/cshim/iMQDestinationShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQDestinationShim.o src/cshim/iMQDestinationShim.cpp

${OBJECTDIR}/src/cshim/iMQLogUtilsShim.o: src/cshim/iMQLogUtilsShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQLogUtilsShim.o src/cshim/iMQLogUtilsShim.cpp

${OBJECTDIR}/src/cshim/iMQMessageShim.o: src/cshim/iMQMessageShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQMessageShim.o src/cshim/iMQMessageShim.cpp

${OBJECTDIR}/src/cshim/iMQProducerShim.o: src/cshim/iMQProducerShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQProducerShim.o src/cshim/iMQProducerShim.cpp

${OBJECTDIR}/src/cshim/iMQPropertiesShim.o: src/cshim/iMQPropertiesShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQPropertiesShim.o src/cshim/iMQPropertiesShim.cpp

${OBJECTDIR}/src/cshim/iMQSSLShim.o: src/cshim/iMQSSLShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQSSLShim.o src/cshim/iMQSSLShim.cpp

${OBJECTDIR}/src/cshim/iMQSessionShim.o: src/cshim/iMQSessionShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQSessionShim.o src/cshim/iMQSessionShim.cpp

${OBJECTDIR}/src/cshim/iMQStatusShim.o: src/cshim/iMQStatusShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQStatusShim.o src/cshim/iMQStatusShim.cpp

${OBJECTDIR}/src/cshim/iMQTextMessageShim.o: src/cshim/iMQTextMessageShim.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQTextMessageShim.o src/cshim/iMQTextMessageShim.cpp

${OBJECTDIR}/src/cshim/iMQTypes.o: src/cshim/iMQTypes.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/iMQTypes.o src/cshim/iMQTypes.cpp

${OBJECTDIR}/src/cshim/shimUtils.o: src/cshim/shimUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/shimUtils.o src/cshim/shimUtils.cpp

${OBJECTDIR}/src/cshim/xaswitch.o: src/cshim/xaswitch.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/cshim
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cshim/xaswitch.o src/cshim/xaswitch.cpp

${OBJECTDIR}/src/error/ErrorCodes.o: src/error/ErrorCodes.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/error
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/error/ErrorCodes.o src/error/ErrorCodes.cpp

${OBJECTDIR}/src/error/ErrorTrace.o: src/error/ErrorTrace.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/error
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/error/ErrorTrace.o src/error/ErrorTrace.cpp

${OBJECTDIR}/src/io/IMQDataInputStream.o: src/io/IMQDataInputStream.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/IMQDataInputStream.o src/io/IMQDataInputStream.cpp

${OBJECTDIR}/src/io/IMQDataOutputStream.o: src/io/IMQDataOutputStream.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/IMQDataOutputStream.o src/io/IMQDataOutputStream.cpp

${OBJECTDIR}/src/io/Packet.o: src/io/Packet.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/Packet.o src/io/Packet.cpp

${OBJECTDIR}/src/io/PacketFlag.o: src/io/PacketFlag.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/PacketFlag.o src/io/PacketFlag.cpp

${OBJECTDIR}/src/io/PacketProperties.o: src/io/PacketProperties.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/PacketProperties.o src/io/PacketProperties.cpp

${OBJECTDIR}/src/io/PacketType.o: src/io/PacketType.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/PacketType.o src/io/PacketType.cpp

${OBJECTDIR}/src/io/PortMapperEntry.o: src/io/PortMapperEntry.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/PortMapperEntry.o src/io/PortMapperEntry.cpp

${OBJECTDIR}/src/io/PortMapperTable.o: src/io/PortMapperTable.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/PortMapperTable.o src/io/PortMapperTable.cpp

${OBJECTDIR}/src/io/SSLSocket.o: src/io/SSLSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/SSLSocket.o src/io/SSLSocket.cpp

${OBJECTDIR}/src/io/SocketTest.o: src/io/SocketTest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/SocketTest.o src/io/SocketTest.cpp

${OBJECTDIR}/src/io/Status.o: src/io/Status.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/Status.o src/io/Status.cpp

${OBJECTDIR}/src/io/SysMessageID.o: src/io/SysMessageID.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/SysMessageID.o src/io/SysMessageID.cpp

${OBJECTDIR}/src/io/TCPSocket.o: src/io/TCPSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/io
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/io/TCPSocket.o src/io/TCPSocket.cpp

${OBJECTDIR}/src/net/IPAddress.o: src/net/IPAddress.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/net
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/net/IPAddress.o src/net/IPAddress.cpp

${OBJECTDIR}/src/serial/SerialDataInputStream.o: src/serial/SerialDataInputStream.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/serial
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/serial/SerialDataInputStream.o src/serial/SerialDataInputStream.cpp

${OBJECTDIR}/src/serial/SerialDataOutputStream.o: src/serial/SerialDataOutputStream.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/serial
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/serial/SerialDataOutputStream.o src/serial/SerialDataOutputStream.cpp

${OBJECTDIR}/src/serial/SerialHandleManager.o: src/serial/SerialHandleManager.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/serial
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/serial/SerialHandleManager.o src/serial/SerialHandleManager.cpp

${OBJECTDIR}/src/serial/Serialize.o: src/serial/Serialize.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/serial
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/serial/Serialize.o src/serial/Serialize.cpp

${OBJECTDIR}/src/util/Logger.o: src/util/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/util
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/util/Logger.o src/util/Logger.cpp

${OBJECTDIR}/src/util/MemAllocTest.o: src/util/MemAllocTest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/util
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/util/MemAllocTest.o src/util/MemAllocTest.cpp

${OBJECTDIR}/src/util/PRTypesUtils.o: src/util/PRTypesUtils.c 
	${MKDIR} -p ${OBJECTDIR}/src/util
	${RM} $@.d
	$(COMPILE.c) -O2 -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/util/PRTypesUtils.o src/util/PRTypesUtils.c

${OBJECTDIR}/src/util/utf8.o: src/util/utf8.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/util
	${RM} $@.d
	$(COMPILE.cc) -O2 -DLINUX -Isrc/include/nss -Isrc/include/nspr -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/util/utf8.o src/util/utf8.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} lib/libopenmqc.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
