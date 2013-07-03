#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which establishes the tcp network connection to
   the GuiServerDevice.
"""

__all__ = ["Network"]


from libkarathon import *
from logindialog import LoginDialog
from manager import Manager
from struct import *

from PyQt4.QtNetwork import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *

import socket

BYTES_TOTAL_SIZE_TAG = 4
BYTES_HEADER_SIZE_TAG = 4
BYTES_MESSAGE_SIZE_TAG = 4

auth = Authenticator("", "", "", "", "", "", "")


class Network(QObject):
           
    def __init__(self):
        super(Network, self).__init__()
        
        self.__textSerializer = TextSerializerHash.create("Xml")
        self.__binarySerializer = BinarySerializerHash.create("Bin")
        
        self.__username = str()
        self.__password = str()
        self.__domain = str()
        self.__sessionToken = str()
        
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
        
        Manager().notifier.signalCreateNewDeviceClassPlugin.connect(self.onCreateNewDeviceClassPlugin)
        
        Manager().notifier.signalGetClassSchema.connect(self.onGetClassSchema)
        Manager().notifier.signalGetDeviceSchema.connect(self.onGetDeviceSchema)
        
        self.__headerSize = 0
        self.__bodySize = 0
        self.__headerBytes = bytearray()
        self.__bodyBytes = bytearray()


    def _login(self, username, password, domain, hostname, portNumber):
        global auth
        
        # System variables definition
        #ipAddress = socket.gethostbyname(socket.gethostname()); #IP
        ipAddress = socket.gethostname(); #Machine Name
        software = "Karabo";
        timeStr = "20130120T122059.259188123";
        #karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);
        
        # Construct Authenticator class
        try:
            auth = Authenticator(username, password, domain, ipAddress, hostname, portNumber, software)
        except Exception, e:
            print "Authenticator exception " + str(e)
            
        # Execute Login
        try:
            return auth.login()
        except Exception, e:
            print "Login exception. Please verify if Service is running!!!" + str(e)
            
    
    def _logout(self):
        # Execute Logout
        try:
            return auth.logout()
        except Exception, e:
            print "Logout exception. Please verify if Service is running!!!" + str(e)


### Slots ###
    def onStartConnection(self):
        dialog = LoginDialog()
        if dialog.exec_() == QDialog.Accepted:
            #if self._login(str(dialog.username), str(dialog.password), str(dialog.domain), str(dialog.hostname), str(dialog.port)):
            #    print "LMAIA: Login successfull!!!"
                # test request to server
                self.__bodySize = 0
                self.__tcpSocket.abort()
                self.__tcpSocket.connectToHost(dialog.hostname, dialog.port)
                
                self.__username = dialog.username
                self.__password = dialog.password
                self.__domain = dialog.domain
                self.__sessionToken = str(auth.getSessionToken())
                #self._sendLoginInformation(dialog.username, dialog.password, dialog.domain, str(auth.getSessionToken()))
            #else:
            #    print "LMAIA: Login error!!!"

    
    def onEndConnection(self):
        if self._logout():
            print "LMAIA: Logout successfull!!!"
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

        print self.__tcpSocket.bytesAvailable(), " bytes are coming in"
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
                
            if self.__bodySize == 0:
            
                if self.__tcpSocket.bytesAvailable() < (BYTES_MESSAGE_SIZE_TAG) :
                    break

                self.__bodySize = input.readUInt32()
                          
            if len(self.__bodyBytes) == 0:
            
                if self.__tcpSocket.bytesAvailable() < self.__bodySize:
                    break
                
                self.__bodyBytes = bytearray(self.__bodySize)
                self.__bodyBytes = input.readRawData(self.__bodySize)
                
                    
            # Fork on responseType
            headerHash = self.__textSerializer.load(self.__headerBytes)
            #bodyHash = self.__textSerializer.load(self.__bodyBytes)
            
            type = headerHash.get("type")
            print "Request: ", type
            
            # "instanceNew" (instanceId, instanceInfo)
            # "instanceUpdated" (instanceId, instanceInfo)
            # "instanceGone" (instanceId)
            # "configurationChanged" (config, instanceId)
            # "log" (logMessage)
            # "notification" (type, shortMsg, detailedMsg, instanceId)
            # "invalidateCache" (instanceId)
            
            if type == "systemTopology":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
            elif type == "instanceNew":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
                
                deviceKey = "device"
                if bodyHash.has(deviceKey):
                    deviceConfig = bodyHash.get(deviceKey)
                    deviceIds = list()
                    deviceConfig.getKeys(deviceIds)
                    for deviceId in deviceIds:
                        Manager().onSelectNewDevice(deviceKey + "." + deviceId)
            elif type == "instanceUpdated":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                print "INCOMING UPDATE "
                print bodyHash
                print ""
                Manager().handleSystemTopology(bodyHash)
            elif type == "instanceGone":
                Manager().handleInstanceGone(str(self.__bodyBytes))
            elif type == "classDescription":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                Manager().handleClassSchema(bodyHash)
            elif type == "deviceSchema":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                self._handleDeviceSchema(headerHash, bodyHash)
            elif type == "configurationChanged":
                bodyHash = self.__textSerializer.load(self.__bodyBytes)
                self._handleConfigurationChanged(headerHash, bodyHash)
            elif type == "log":
                self._handleLog(str(self.__bodyBytes))
            elif type == "notification":
                print "notification"
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
        self._sendLoginInformation(self.__username, self.__password, self.__domain, self.__sessionToken)
        
        
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
        header.set("instanceId", str(instanceId))
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
        
        command = info.get(QString('command'))
        if command is None:
            command = info.get('command')
        body = Hash("command", str(command))
        
        args = info.get(QString('args'))
        if args is None:
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
    def _sendLoginInformation(self, username, password, domain, sessionToken):
        header = Hash("type", "login")
        body = Hash()
        body.set("username", str(username))
        body.set("password", str(password))
        body.set("domain", str(domain))
        body.set("sessionToken", str(sessionToken))
        
        self._tcpWriteHashHash(header, body)


    def _tcpWriteHashHash(self, headerHash, bodyHash):
        stream = QByteArray()
        headerString = QByteArray(self.__textSerializer.save(headerHash))
        bodyString = QByteArray(self.__textSerializer.save(bodyHash))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()
                
        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(QByteArray(pack('I', nBytesBody)))
        stream.push_back(bodyString)
        self.__tcpSocket.write(stream)
        
        
    def _tcpWriteHashString(self, headerHash, body):
        stream = QByteArray()
        headerString = QByteArray(self.__textSerializer.save(headerHash))
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

