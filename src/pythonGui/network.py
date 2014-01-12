#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which establishes the tcp network connection to
   the GuiServerDevice.
"""

__all__ = ["Network"]


from karabo.karathon import *
from manager import Manager
from struct import *

from PyQt4.QtNetwork import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *

import globals
import socket

BYTES_TOTAL_SIZE_TAG = 4
BYTES_HEADER_SIZE_TAG = 4
BYTES_MESSAGE_SIZE_TAG = 4

class Network(QObject):
    # signals
    signalUserChanged = pyqtSignal()
           
    def __init__(self):
        super(Network, self).__init__()
        
        self.__serializer = BinarySerializerHash.create("Bin")
                
        self.__auth = None
        self.__username = str()
        self.__password = str()
        self.__provider = str()
        self.__sessionToken = str()
        self.__brokerHost = str()
        self.__brokerPort = str()
        self.__brokerTopic = str()
        
        self.__tcpSocket = QTcpSocket(self)
        self.__tcpSocket.connected.connect(self.onConnected)
        self.__tcpSocket.disconnected.connect(self.onDisconnected)
        self.__tcpSocket.readyRead.connect(self.onReadServerData)
        self.__tcpSocket.error.connect(self.onDisplayServerError)
        
        Manager().notifier.signalKillDevice.connect(self.onKillDevice)
        Manager().notifier.signalKillServer.connect(self.onKillServer)
        Manager().notifier.signalRefreshInstance.connect(self.onRefreshInstance)
        Manager().notifier.signalReconfigure.connect(self.onReconfigure)
        Manager().notifier.signalReconfigureAsHash.connect(self.onReconfigureAsHash)
        Manager().notifier.signalInitDevice.connect(self.onInitDevice)
        Manager().notifier.signalExecute.connect(self.onExecute)
        
        Manager().notifier.signalNewVisibleDevice.connect(self.onNewVisibleDevice)
        Manager().notifier.signalRemoveVisibleDevice.connect(self.onRemoveVisibleDevice)
        
        Manager().notifier.signalGetClassSchema.connect(self.onGetClassSchema)
        Manager().notifier.signalGetDeviceSchema.connect(self.onGetDeviceSchema)
        
        self.__headerSize = 0
        self.__bodySize = 0
        self.__headerBytes = bytearray()
        self.__bodyBytes = bytearray()


    def login(self):
       
        # System variables definition
        #ipAddress = socket.gethostbyname(socket.gethostname()); #IP
        ipAddress = socket.gethostname(); #Machine Name
        
        print "Trying to login now..."
        
        # Easteregg
        if self.__username == "god":
            md5 = QCryptographicHash.hash(str(self.__password), QCryptographicHash.Md5).toHex()
            if md5 == "39d676ecced45b02da1fb45731790b4c":
                print "Entering god mode..."
                globals.GLOBAL_ACCESS_LEVEL = 1000
            else:
                globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
                
        else:
        
            # Construct Authenticator class
            try:
                self.__auth = Authenticator(self.__username, self.__password, self.__provider, ipAddress, self.__brokerHost, self.__brokerPort, self.__brokerTopic)
            except Exception, e:
                print "Authenticator exception " + str(e)

            # Execute Login
            ok = False
            try:
                ok = self.__auth.login()
            except Exception, e:
                # TODO Fall back to inbuild access level
                globals.GLOBAL_ACCESS_LEVEL = globals.KARABO_DEFAULT_ACCESS_LEVEL
                print "Login exception. Please verify if Service is running!!!" + str(e)
                
                # Inform the mainwindow to change correspondingly the allowed level-downgrade
                self.signalUserChanged.emit()
                self._sendLoginInformation(self.__username, self.__password, self.__provider, self.__sessionToken)
                return
            
            if ok:
                print "Login successful"
                globals.GLOBAL_ACCESS_LEVEL = self.__auth.getDefaultAccessLevelId()
            else:
                print "Login failed"
                globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER

        # Inform the mainwindow to change correspondingly the allowed level-downgrade
        self.signalUserChanged.emit()
        self._sendLoginInformation(self.__username, self.__password, self.__provider, self.__sessionToken)
            
    
    def _logout(self):
        # Execute Logout
        try:
            return self.__auth.logout()
        except Exception, e:
            print "Logout exception. Please verify if Service is running!!!" + str(e)


### Slots ###
    def onStartConnection(self, username, password, provider, hostname, port):
        
        self.__username = username
        self.__password = password
        self.__provider = provider
        self.__headerSize = 0
        self.__bodySize = 0
        self.__tcpSocket.abort()
        self.__tcpSocket.connectToHost(hostname, port)


    def onEndConnection(self):
        if self._logout():
            print "LMAIA: Logout successful!!!"
            Manager().closeDatabaseConnection()
            self.__tcpSocket.disconnectFromHost()
            if self.__tcpSocket.state() == QAbstractSocket.UnconnectedState or self.__tcpSocket.waitForDisconnected(1000):
                print "Disconnected from server"
            else:
                print "Disconnect failed:", self.__tcpSocket.errorString()
        else:
            print "LMAIA: Logout error!!!"

                
    def onReadServerData(self):
        input = QDataStream(self.__tcpSocket)
        input.setByteOrder(QDataStream.LittleEndian)

        #print self.__tcpSocket.bytesAvailable(), " bytes are coming in"
        while True:

            if self.__headerSize == 0:
            
                if self.__tcpSocket.bytesAvailable() < (BYTES_MESSAGE_SIZE_TAG) :
                    break

                self.__headerSize = input.readUInt32()
                          
            if len(self.__headerBytes) == 0:
            
                if self.__tcpSocket.bytesAvailable() < self.__headerSize :
                    break
                
                self.__headerBytes = bytearray(self.__headerSize)
                self.__headerBytes = input.readRawData(self.__headerSize)
                # TODO How to do this nicely?
                self.__headerBytes = bytearray(self.__headerBytes)
                
            if self.__bodySize == 0:
            
                if self.__tcpSocket.bytesAvailable() < (BYTES_MESSAGE_SIZE_TAG) :
                    break

                self.__bodySize = input.readUInt32()
                          
            if len(self.__bodyBytes) == 0:
            
                if self.__tcpSocket.bytesAvailable() < self.__bodySize:
                    break
                
                self.__bodyBytes = bytearray(self.__bodySize)
                self.__bodyBytes = input.readRawData(self.__bodySize)
                # TODO How to do this nicely?
                self.__bodyBytes = bytearray(self.__bodyBytes)
                
                    
            # Fork on responseType
            headerHash = self.__serializer.load(self.__headerBytes)
            #bodyHash = self.__serializer.load(self.__bodyBytes)
            
            type = headerHash.get("type")
            #print "Request: ", type
            
            # "instanceNew" (instanceId, instanceInfo)
            # "instanceUpdated" (instanceId, instanceInfo)
            # "instanceGone" (instanceId)
            # "configurationChanged" (config, instanceId)
            # "log" (logMessage)
            # "notification" (type, shortMsg, detailedMsg, instanceId)
            # "invalidateCache" (instanceId)
            
            if type == "systemTopology":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
            elif type == "instanceNew":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
                
                deviceKey = "device"
                if bodyHash.has(deviceKey):
                    deviceConfig = bodyHash.get(deviceKey)
                    deviceIds = list()
                    deviceConfig.getKeys(deviceIds)
                    for deviceId in deviceIds:
                        Manager().onSelectNewDevice(deviceKey + "." + deviceId)
            elif type == "instanceUpdated":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
            elif type == "instanceGone":
                Manager().handleInstanceGone(str(self.__bodyBytes))
            elif type == "classDescription":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleClassSchema(bodyHash)
            elif type == "deviceSchema":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleDeviceSchema(headerHash, bodyHash)
            elif type == "configurationChanged":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleConfigurationChanged(headerHash, bodyHash)
            elif type == "log":
                self._handleLog(str(self.__bodyBytes))
            elif type == "schemaUpdated":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleSchemaUpdated(headerHash, bodyHash)
            elif type == "brokerInformation":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleBrokerInformation(headerHash, bodyHash)
            elif type == "notification":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleNotification(headerHash, bodyHash)
            elif type == "invalidateCache":
                print "invalidateCache"
            else:
                print "WARN : Got unknown communication token \"", type, "\" from server"
            # Invalidate variables            
            self.__bodySize = self.__headerSize = 0
            self.__headerBytes = self.__bodyBytes = bytearray()


    def onDisplayServerError(self, socketError):
        print "onDisplayServerError", self.__tcpSocket.errorString()


    def onConnected(self):
        print "Connected to server"
        #self._sendLoginInformation(self.__username, self.__password, self.__provider, self.__sessionToken)
        
        
    def onDisconnected(self):
        print "Disconnected from server"
        pass


    def onKillDevice(self, deviceId):
        header = Hash()
        header.set("type", "killDevice")
        header.set("deviceId", str(deviceId));
        self._tcpWriteHashHash(header, Hash())


    def onKillServer(self, serverId):
        header = Hash()
        header.set("type", "killServer")
        header.set("serverId", str(serverId))
        self._tcpWriteHashHash(header, Hash())


    def onRefreshInstance(self, instanceId):
        header = Hash()
        header.set("type", "refreshInstance")
        header.set("deviceId", str(instanceId))
        self._tcpWriteHashHash(header, Hash())
        
        
    def onReconfigure(self, deviceId, parameterId, value):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("deviceId", str(deviceId))
        body = Hash()
        body.set(str(parameterId), value)
        self._tcpWriteHashHash(header, body)


    def onReconfigureAsHash(self, deviceId, body):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("deviceId", str(deviceId))
        self._tcpWriteHashHash(header, body)


    def onInitDevice(self, serverId, config):
        header = Hash()
        header.set("type", "initDevice")
        header.set("serverId", str(serverId))
        self._tcpWriteHashHash(header, config)


    def onCreateNewDeviceClassPlugin(self, devSrvInsId, classId, newClassId):
        header = Hash("type", "createNewDeviceClassPlugin")
        body = Hash()
        body.set("devSrvInsId", str(devSrvInsId))
        body.set("classId", str(classId))
        body.set("newClassId", str(newClassId))
        self._tcpWriteHashHash(header, body)


    def onExecute(self, deviceId, info):
        header = Hash()
        header.set("type", "execute")
        header.set("deviceId", str(deviceId))
        
        command = info.get('command')
        body = Hash("command", str(command))
        
        args = info.get('args')
        if args:
            i = 0
            for arg in args:
                i = i+1
                argName = str("a%s" % i)
                body.set(argName, str(arg))
        self._tcpWriteHashHash(header, body)


    def onNewVisibleDevice(self, deviceId):
        header = Hash()
        header.set("type", "newVisibleDevice")
        header.set("deviceId", str(deviceId))
        self._tcpWriteHashHash(header, Hash())


    def onRemoveVisibleDevice(self, deviceId):
        header = Hash()
        header.set("type", "removeVisibleDevice")
        header.set("deviceId", str(deviceId))
        self._tcpWriteHashHash(header, Hash())


    def onGetClassSchema(self, serverId, classId):
        header = Hash("type", "getClassSchema")
        header.set("serverId", str(serverId))
        header.set("classId", str(classId))
        self._tcpWriteHashHash(header, Hash())


    def onGetDeviceSchema(self, deviceId):
        header = Hash("type", "getDeviceSchema")
        header.set("deviceId", str(deviceId))
        self._tcpWriteHashHash(header, Hash())


### private functions ###
    def _sendLoginInformation(self, username, password, provider, sessionToken):
        header = Hash("type", "login")
        body = Hash()
        body.set("username", str(username))
        body.set("password", str(password))
        body.set("provider", str(provider))
        body.set("sessionToken", str(sessionToken))
        
        self._tcpWriteHashHash(header, body)


    def _tcpWriteHashHash(self, headerHash, bodyHash):
        stream = QByteArray()
        headerString = QByteArray(self.__serializer.save(headerHash))
        bodyString = QByteArray(self.__serializer.save(bodyHash))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()
                
        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(QByteArray(pack('I', nBytesBody)))
        stream.push_back(bodyString)
        self.__tcpSocket.write(stream)
        
        
    def _tcpWriteHashString(self, headerHash, body):
        stream = QByteArray()
        headerString = QByteArray(self.__serializer.save(headerHash))
        bodyString = QByteArray(str(body))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()
                
        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(QByteArray(pack('I', nBytesBody)))
        stream.push_back(bodyString)
        self.__tcpSocket.write(stream)


    def _handleLog(self, logMessage):
        Manager().onLogDataAvailable(logMessage)


    def _handleDeviceSchema(self, headerHash, bodyHash):
        deviceId = headerHash.get("deviceId")
        Manager().handleDeviceSchema(deviceId, bodyHash)


    def _handleConfigurationChanged(self, headerHash, bodyHash):
        deviceId = headerHash.get("deviceId")        
        Manager().handleConfigurationChanged(deviceId, bodyHash)
        
        
    def _handleSchemaUpdated(self, headerHash, bodyHash):
        deviceId = headerHash.get("deviceId")
        Manager().handleDeviceSchemaUpdated(deviceId, bodyHash)


    def _handleBrokerInformation(self, headerHash, bodyHash):
        self.__brokerHost = bodyHash.get("host")
        self.__brokerPort = str(bodyHash.get("port"))
        self.__brokerTopic = bodyHash.get("topic")
        self.login()


    def _handleNotification(self, headerHash, bodyHash):
        deviceId = headerHash.get("deviceId")
        timestamp = Timestamp()
        # TODO: better format for timestamp and timestamp generation in karabo
        timestamp = timestamp.toFormattedString("%Y-%m-%d %H:%M:%S")
        type = bodyHash.get("type")
        shortMessage = bodyHash.get("shortMsg")
        detailedMessage = bodyHash.get("detailedMsg")
        Manager().handleNotification(timestamp, type, shortMessage, detailedMessage, deviceId)

