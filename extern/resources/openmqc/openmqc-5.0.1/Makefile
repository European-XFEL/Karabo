
# Environment
MKDIR=mkdir
CP=cp
RM=rm -f
CC=gcc
CXX=g++

# Link Libraries and Options
LDLIBSOPTIONS=-L${EXTERN_DISTDIR}/lib -Wl,-rpath,\$$ORIGIN -lssl3 -lsmime3 -lnssutil3 -lnss3 -lplds4 -lplc4 -lnspr4

# CC Compiler Flags
INCLUDES=-I${EXTERN_DISTDIR}/include/nss -I${EXTERN_DISTDIR}/include/nspr
CCFLAGS=-c -O2 ${INCLUDES} -fPIC
CXXFLAGS=-c -O2 -DLINUX ${INCLUDES} -fPIC

# Object Files
OBJECTFILES= \
	build/src/basictypes/BasicType.o \
	build/src/basictypes/Boolean.o \
	build/src/basictypes/Byte.o \
	build/src/basictypes/Double.o \
	build/src/basictypes/Float.o \
	build/src/basictypes/HandledObject.o \
	build/src/basictypes/Integer.o \
	build/src/basictypes/Long.o \
	build/src/basictypes/Monitor.o \
	build/src/basictypes/Object.o \
	build/src/basictypes/Short.o \
	build/src/basictypes/UTF8String.o \
	build/src/client/BytesMessage.o \
	build/src/client/Connection.o \
	build/src/client/Destination.o \
	build/src/client/FlowControl.o \
	build/src/client/Message.o \
	build/src/client/MessageConsumer.o \
	build/src/client/MessageConsumerTable.o \
	build/src/client/MessageID.o \
	build/src/client/MessageProducer.o \
	build/src/client/NSSInitCall.o \
	build/src/client/PingTimer.o \
	build/src/client/PortMapperClient.o \
	build/src/client/ProducerFlow.o \
	build/src/client/ProtocolHandler.o \
	build/src/client/ReadChannel.o \
	build/src/client/ReadQTable.o \
	build/src/client/ReceiveQueue.o \
	build/src/client/Session.o \
	build/src/client/SessionMutex.o \
	build/src/client/SessionQueueReader.o \
	build/src/client/TextMessage.o \
	build/src/client/XASession.o \
	build/src/client/XIDObject.o \
	build/src/client/auth/JMQBasicAuthenticationHandler.o \
	build/src/client/auth/JMQDigestAuthenticationHandler.o \
	build/src/client/protocol/SSLProtocolHandler.o \
	build/src/client/protocol/StubProtocolHandler.o \
	build/src/client/protocol/TCPProtocolHandler.o \
	build/src/containers/BasicTypeHashtable.o \
	build/src/containers/ObjectVector.o \
	build/src/containers/Properties.o \
	build/src/containers/StringKeyHashtable.o \
	build/src/containers/Vector.o \
	build/src/cshim/iMQBytesMessageShim.o \
	build/src/cshim/iMQCallbacks.o \
	build/src/cshim/iMQConnectionShim.o \
	build/src/cshim/iMQConsumerShim.o \
	build/src/cshim/iMQDestinationShim.o \
	build/src/cshim/iMQLogUtilsShim.o \
	build/src/cshim/iMQMessageShim.o \
	build/src/cshim/iMQProducerShim.o \
	build/src/cshim/iMQPropertiesShim.o \
	build/src/cshim/iMQSSLShim.o \
	build/src/cshim/iMQSessionShim.o \
	build/src/cshim/iMQStatusShim.o \
	build/src/cshim/iMQTextMessageShim.o \
	build/src/cshim/iMQTypes.o \
	build/src/cshim/shimUtils.o \
	build/src/cshim/xaswitch.o \
	build/src/error/ErrorCodes.o \
	build/src/error/ErrorTrace.o \
	build/src/io/IMQDataInputStream.o \
	build/src/io/IMQDataOutputStream.o \
	build/src/io/Packet.o \
	build/src/io/PacketFlag.o \
	build/src/io/PacketProperties.o \
	build/src/io/PacketType.o \
	build/src/io/PortMapperEntry.o \
	build/src/io/PortMapperTable.o \
	build/src/io/SSLSocket.o \
	build/src/io/SocketTest.o \
	build/src/io/Status.o \
	build/src/io/SysMessageID.o \
	build/src/io/TCPSocket.o \
	build/src/net/IPAddress.o \
	build/src/serial/SerialDataInputStream.o \
	build/src/serial/SerialDataOutputStream.o \
	build/src/serial/SerialHandleManager.o \
	build/src/serial/Serialize.o \
	build/src/util/Logger.o \
	build/src/util/MemAllocTest.o \
	build/src/util/PRTypesUtils.o \
	build/src/util/utf8.o


lib/libopenmqc.so: ${OBJECTFILES}
	${MKDIR} -p lib
	${CXX} -o lib/libopenmqc.so ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC
	${MKDIR} -p include/openmqc
	${CP} src/cshim/*.h include/openmqc


build/src/basictypes/BasicType.o: src/basictypes/BasicType.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/BasicType.o src/basictypes/BasicType.cpp

build/src/basictypes/Boolean.o: src/basictypes/Boolean.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Boolean.o src/basictypes/Boolean.cpp

build/src/basictypes/Byte.o: src/basictypes/Byte.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Byte.o src/basictypes/Byte.cpp

build/src/basictypes/Double.o: src/basictypes/Double.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Double.o src/basictypes/Double.cpp

build/src/basictypes/Float.o: src/basictypes/Float.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Float.o src/basictypes/Float.cpp

build/src/basictypes/HandledObject.o: src/basictypes/HandledObject.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/HandledObject.o src/basictypes/HandledObject.cpp

build/src/basictypes/Integer.o: src/basictypes/Integer.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Integer.o src/basictypes/Integer.cpp

build/src/basictypes/Long.o: src/basictypes/Long.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Long.o src/basictypes/Long.cpp

build/src/basictypes/Monitor.o: src/basictypes/Monitor.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Monitor.o src/basictypes/Monitor.cpp

build/src/basictypes/Object.o: src/basictypes/Object.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Object.o src/basictypes/Object.cpp

build/src/basictypes/Short.o: src/basictypes/Short.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/Short.o src/basictypes/Short.cpp

build/src/basictypes/UTF8String.o: src/basictypes/UTF8String.cpp
	${MKDIR} -p build/src/basictypes
	${CXX} ${CXXFLAGS} -o build/src/basictypes/UTF8String.o src/basictypes/UTF8String.cpp

build/src/client/BytesMessage.o: src/client/BytesMessage.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/BytesMessage.o src/client/BytesMessage.cpp

build/src/client/Connection.o: src/client/Connection.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/Connection.o src/client/Connection.cpp

build/src/client/Destination.o: src/client/Destination.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/Destination.o src/client/Destination.cpp

build/src/client/FlowControl.o: src/client/FlowControl.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/FlowControl.o src/client/FlowControl.cpp

build/src/client/Message.o: src/client/Message.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/Message.o src/client/Message.cpp

build/src/client/MessageConsumer.o: src/client/MessageConsumer.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/MessageConsumer.o src/client/MessageConsumer.cpp

build/src/client/MessageConsumerTable.o: src/client/MessageConsumerTable.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/MessageConsumerTable.o src/client/MessageConsumerTable.cpp

build/src/client/MessageID.o: src/client/MessageID.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/MessageID.o src/client/MessageID.cpp

build/src/client/MessageProducer.o: src/client/MessageProducer.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/MessageProducer.o src/client/MessageProducer.cpp

build/src/client/NSSInitCall.o: src/client/NSSInitCall.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/NSSInitCall.o src/client/NSSInitCall.cpp

build/src/client/PingTimer.o: src/client/PingTimer.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/PingTimer.o src/client/PingTimer.cpp

build/src/client/PortMapperClient.o: src/client/PortMapperClient.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/PortMapperClient.o src/client/PortMapperClient.cpp

build/src/client/ProducerFlow.o: src/client/ProducerFlow.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/ProducerFlow.o src/client/ProducerFlow.cpp

build/src/client/ProtocolHandler.o: src/client/ProtocolHandler.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/ProtocolHandler.o src/client/ProtocolHandler.cpp

build/src/client/ReadChannel.o: src/client/ReadChannel.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/ReadChannel.o src/client/ReadChannel.cpp

build/src/client/ReadQTable.o: src/client/ReadQTable.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/ReadQTable.o src/client/ReadQTable.cpp

build/src/client/ReceiveQueue.o: src/client/ReceiveQueue.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/ReceiveQueue.o src/client/ReceiveQueue.cpp

build/src/client/Session.o: src/client/Session.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/Session.o src/client/Session.cpp

build/src/client/SessionMutex.o: src/client/SessionMutex.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/SessionMutex.o src/client/SessionMutex.cpp

build/src/client/SessionQueueReader.o: src/client/SessionQueueReader.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/SessionQueueReader.o src/client/SessionQueueReader.cpp

build/src/client/TextMessage.o: src/client/TextMessage.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/TextMessage.o src/client/TextMessage.cpp

build/src/client/XASession.o: src/client/XASession.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/XASession.o src/client/XASession.cpp

build/src/client/XIDObject.o: src/client/XIDObject.cpp
	${MKDIR} -p build/src/client
	${CXX} ${CXXFLAGS} -o build/src/client/XIDObject.o src/client/XIDObject.cpp

build/src/client/auth/JMQBasicAuthenticationHandler.o: src/client/auth/JMQBasicAuthenticationHandler.cpp
	${MKDIR} -p build/src/client/auth
	${CXX} ${CXXFLAGS} -o build/src/client/auth/JMQBasicAuthenticationHandler.o src/client/auth/JMQBasicAuthenticationHandler.cpp

build/src/client/auth/JMQDigestAuthenticationHandler.o: src/client/auth/JMQDigestAuthenticationHandler.cpp
	${MKDIR} -p build/src/client/auth
	${CXX} ${CXXFLAGS} -o build/src/client/auth/JMQDigestAuthenticationHandler.o src/client/auth/JMQDigestAuthenticationHandler.cpp

build/src/client/protocol/SSLProtocolHandler.o: src/client/protocol/SSLProtocolHandler.cpp
	${MKDIR} -p build/src/client/protocol
	${CXX} ${CXXFLAGS} -o build/src/client/protocol/SSLProtocolHandler.o src/client/protocol/SSLProtocolHandler.cpp

build/src/client/protocol/StubProtocolHandler.o: src/client/protocol/StubProtocolHandler.cpp
	${MKDIR} -p build/src/client/protocol
	${CXX} ${CXXFLAGS} -o build/src/client/protocol/StubProtocolHandler.o src/client/protocol/StubProtocolHandler.cpp

build/src/client/protocol/TCPProtocolHandler.o: src/client/protocol/TCPProtocolHandler.cpp
	${MKDIR} -p build/src/client/protocol
	${CXX} ${CXXFLAGS} -o build/src/client/protocol/TCPProtocolHandler.o src/client/protocol/TCPProtocolHandler.cpp

build/src/containers/BasicTypeHashtable.o: src/containers/BasicTypeHashtable.cpp
	${MKDIR} -p build/src/containers
	${CXX} ${CXXFLAGS} -o build/src/containers/BasicTypeHashtable.o src/containers/BasicTypeHashtable.cpp

build/src/containers/ObjectVector.o: src/containers/ObjectVector.cpp
	${MKDIR} -p build/src/containers
	${CXX} ${CXXFLAGS} -o build/src/containers/ObjectVector.o src/containers/ObjectVector.cpp

build/src/containers/Properties.o: src/containers/Properties.cpp
	${MKDIR} -p build/src/containers
	${CXX} ${CXXFLAGS} -o build/src/containers/Properties.o src/containers/Properties.cpp

build/src/containers/StringKeyHashtable.o: src/containers/StringKeyHashtable.cpp
	${MKDIR} -p build/src/containers
	${CXX} ${CXXFLAGS} -o build/src/containers/StringKeyHashtable.o src/containers/StringKeyHashtable.cpp

build/src/containers/Vector.o: src/containers/Vector.cpp
	${MKDIR} -p build/src/containers
	${CXX} ${CXXFLAGS} -o build/src/containers/Vector.o src/containers/Vector.cpp

build/src/cshim/iMQBytesMessageShim.o: src/cshim/iMQBytesMessageShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQBytesMessageShim.o src/cshim/iMQBytesMessageShim.cpp

build/src/cshim/iMQCallbacks.o: src/cshim/iMQCallbacks.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQCallbacks.o src/cshim/iMQCallbacks.cpp

build/src/cshim/iMQConnectionShim.o: src/cshim/iMQConnectionShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQConnectionShim.o src/cshim/iMQConnectionShim.cpp

build/src/cshim/iMQConsumerShim.o: src/cshim/iMQConsumerShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQConsumerShim.o src/cshim/iMQConsumerShim.cpp

build/src/cshim/iMQDestinationShim.o: src/cshim/iMQDestinationShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQDestinationShim.o src/cshim/iMQDestinationShim.cpp

build/src/cshim/iMQLogUtilsShim.o: src/cshim/iMQLogUtilsShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQLogUtilsShim.o src/cshim/iMQLogUtilsShim.cpp

build/src/cshim/iMQMessageShim.o: src/cshim/iMQMessageShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQMessageShim.o src/cshim/iMQMessageShim.cpp

build/src/cshim/iMQProducerShim.o: src/cshim/iMQProducerShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQProducerShim.o src/cshim/iMQProducerShim.cpp

build/src/cshim/iMQPropertiesShim.o: src/cshim/iMQPropertiesShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQPropertiesShim.o src/cshim/iMQPropertiesShim.cpp

build/src/cshim/iMQSSLShim.o: src/cshim/iMQSSLShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQSSLShim.o src/cshim/iMQSSLShim.cpp

build/src/cshim/iMQSessionShim.o: src/cshim/iMQSessionShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQSessionShim.o src/cshim/iMQSessionShim.cpp

build/src/cshim/iMQStatusShim.o: src/cshim/iMQStatusShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQStatusShim.o src/cshim/iMQStatusShim.cpp

build/src/cshim/iMQTextMessageShim.o: src/cshim/iMQTextMessageShim.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQTextMessageShim.o src/cshim/iMQTextMessageShim.cpp

build/src/cshim/iMQTypes.o: src/cshim/iMQTypes.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/iMQTypes.o src/cshim/iMQTypes.cpp

build/src/cshim/shimUtils.o: src/cshim/shimUtils.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/shimUtils.o src/cshim/shimUtils.cpp

build/src/cshim/xaswitch.o: src/cshim/xaswitch.cpp
	${MKDIR} -p build/src/cshim
	${CXX} ${CXXFLAGS} -o build/src/cshim/xaswitch.o src/cshim/xaswitch.cpp

build/src/error/ErrorCodes.o: src/error/ErrorCodes.cpp
	${MKDIR} -p build/src/error
	${CXX} ${CXXFLAGS} -o build/src/error/ErrorCodes.o src/error/ErrorCodes.cpp

build/src/error/ErrorTrace.o: src/error/ErrorTrace.cpp
	${MKDIR} -p build/src/error
	${CXX} ${CXXFLAGS} -o build/src/error/ErrorTrace.o src/error/ErrorTrace.cpp

build/src/io/IMQDataInputStream.o: src/io/IMQDataInputStream.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/IMQDataInputStream.o src/io/IMQDataInputStream.cpp

build/src/io/IMQDataOutputStream.o: src/io/IMQDataOutputStream.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/IMQDataOutputStream.o src/io/IMQDataOutputStream.cpp

build/src/io/Packet.o: src/io/Packet.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/Packet.o src/io/Packet.cpp

build/src/io/PacketFlag.o: src/io/PacketFlag.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/PacketFlag.o src/io/PacketFlag.cpp

build/src/io/PacketProperties.o: src/io/PacketProperties.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/PacketProperties.o src/io/PacketProperties.cpp

build/src/io/PacketType.o: src/io/PacketType.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/PacketType.o src/io/PacketType.cpp

build/src/io/PortMapperEntry.o: src/io/PortMapperEntry.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/PortMapperEntry.o src/io/PortMapperEntry.cpp

build/src/io/PortMapperTable.o: src/io/PortMapperTable.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/PortMapperTable.o src/io/PortMapperTable.cpp

build/src/io/SSLSocket.o: src/io/SSLSocket.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/SSLSocket.o src/io/SSLSocket.cpp

build/src/io/SocketTest.o: src/io/SocketTest.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/SocketTest.o src/io/SocketTest.cpp

build/src/io/Status.o: src/io/Status.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/Status.o src/io/Status.cpp

build/src/io/SysMessageID.o: src/io/SysMessageID.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/SysMessageID.o src/io/SysMessageID.cpp

build/src/io/TCPSocket.o: src/io/TCPSocket.cpp
	${MKDIR} -p build/src/io
	${CXX} ${CXXFLAGS} -o build/src/io/TCPSocket.o src/io/TCPSocket.cpp

build/src/net/IPAddress.o: src/net/IPAddress.cpp
	${MKDIR} -p build/src/net
	${CXX} ${CXXFLAGS} -o build/src/net/IPAddress.o src/net/IPAddress.cpp

build/src/serial/SerialDataInputStream.o: src/serial/SerialDataInputStream.cpp
	${MKDIR} -p build/src/serial
	${CXX} ${CXXFLAGS} -o build/src/serial/SerialDataInputStream.o src/serial/SerialDataInputStream.cpp

build/src/serial/SerialDataOutputStream.o: src/serial/SerialDataOutputStream.cpp
	${MKDIR} -p build/src/serial
	${CXX} ${CXXFLAGS} -o build/src/serial/SerialDataOutputStream.o src/serial/SerialDataOutputStream.cpp

build/src/serial/SerialHandleManager.o: src/serial/SerialHandleManager.cpp
	${MKDIR} -p build/src/serial
	${CXX} ${CXXFLAGS} -o build/src/serial/SerialHandleManager.o src/serial/SerialHandleManager.cpp

build/src/serial/Serialize.o: src/serial/Serialize.cpp
	${MKDIR} -p build/src/serial
	${CXX} ${CXXFLAGS} -o build/src/serial/Serialize.o src/serial/Serialize.cpp

build/src/util/Logger.o: src/util/Logger.cpp
	${MKDIR} -p build/src/util
	${CXX} ${CXXFLAGS} -o build/src/util/Logger.o src/util/Logger.cpp

build/src/util/MemAllocTest.o: src/util/MemAllocTest.cpp
	${MKDIR} -p build/src/util
	${CXX} ${CXXFLAGS} -o build/src/util/MemAllocTest.o src/util/MemAllocTest.cpp

build/src/util/PRTypesUtils.o: src/util/PRTypesUtils.c
	${MKDIR} -p build/src/util
	${CC} ${CCFLAGS} -o build/src/util/PRTypesUtils.o src/util/PRTypesUtils.c

build/src/util/utf8.o: src/util/utf8.cpp
	${MKDIR} -p build/src/util
	${CXX} ${CXXFLAGS} -o build/src/util/utf8.o src/util/utf8.cpp


all: lib/libopenmqc.so

clean:
	${RM} -r build/ include/ lib/
