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


BYTES_TOTAL_SIZE_TAG = 4
BYTES_HEADER_SIZE_TAG = 4
BYTES_MESSAGE_SIZE_TAG = 4

class Network(QObject):
           
    def __init__(self):
        super(Network, self).__init__()
        
        self.__textSerializer = TextSerializerHash.create("Xml")
        self.__binarySerializer = BinarySerializerHash.create("Bin")
        
        self.__tcpSocket = QTcpSocket(self)
        self.__tcpSocket.connected.connect(self.onConnected)
        self.__tcpSocket.disconnected.connect(self.onDisconnected)
        self.__tcpSocket.readyRead.connect(self.onReadServerData)
        self.__tcpSocket.error.connect(self.onDisplayServerError)
        
        #Manager().notifier.signalKillDeviceInstance.connect(self.onKillDeviceInstance)
        #Manager().notifier.signalKillDeviceServerInstance.connect(self.onKillDeviceServerInstance)
        #Manager().notifier.signalRefreshInstance.connect(self.onRefreshInstance)
        #Manager().notifier.signalReconfigure.connect(self.onReconfigure)
        #Manager().notifier.signalReconfigureAsHash.connect(self.onReconfigureAsHash)
        #Manager().notifier.signalInitDevice.connect(self.onInitDevice)
        #Manager().notifier.signalSlotCommand.connect(self.onSlotCommand)
        
        #Manager().notifier.signalNewVisibleDeviceInstance.connect(self.onNewVisibleDeviceInstance)
        #Manager().notifier.signalRemoveVisibleDeviceInstance.connect(self.onRemoveVisibleDeviceInstance)
        
        #Manager().notifier.signalCreateNewDeviceClassPlugin.connect(self.onCreateNewDeviceClassPlugin)
        
        self.__headerSize = 0
        self.__bodySize = 0
        self.__headerBytes = bytearray()
        self.__bodyBytes = bytearray()


### Slots ###
    def onStartConnection(self):
        dialog = LoginDialog()
        if dialog.exec_() == QDialog.Accepted:
            # test request to server
            self.__bodySize = 0
            self.__tcpSocket.abort()
            self.__tcpSocket.connectToHost(dialog.hostname, dialog.port)
            self._sendLoginInformation(dialog.username, dialog.password)

    
    def onEndConnection(self):
        Manager().closeDatabaseConnection()
        self.__tcpSocket.disconnectFromHost()
        if self.__tcpSocket.state() == QAbstractSocket.UnconnectedState or self.__tcpSocket.waitForDisconnected(1000):
            print "Disconnected from server"
        else:
            print "Disconnect failed:", self.__tcpSocket.errorString()

                
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
            # "configurationChange" (config, instanceId)
            # "log" (logMessage)
            # "notify" (instanceId, type, text)
            # "invalidateCache" (instanceId)
            
            if type == "instanceNew":
                print "instanceNew"
            elif type == "instanceUpdated":
                print "instanceUpdated"
            elif type == "instanceGone":
                print "instanceGone"
            elif type == "configurationChanged":
                print "configurationChanged"
            elif type == "log":
                print "log"
            elif type == "notify":
                print "notify"
            elif type == "invalidateCache":
                print "invalidateCache"
            
            # Old stuff with test hash...
            if type == "currentInstances":
                # Get system configuration
                testConfig = Hash()
                testConfig.set("server.HEGEL_DeviceServer_0.classes.Conveyor", Hash())
                #testConfig.set("server.HEGEL_DeviceServer_0.classes.Conveyor.description", Schema())
                #testConfig.set("server.HEGEL_DeviceServer_0.classes.Conveyor.configuration", Hash())

                testConfig.setAttribute("server.HEGEL_DeviceServer_0", "host", "HEGEL")
                testConfig.setAttribute("server.HEGEL_DeviceServer_0", "status", "on")

                testConfig.set("server.HEGEL_DeviceServer_1.classes.HelloWorld", Hash())
                testConfig.setAttribute("server.HEGEL_DeviceServer_1", "host", "HEGEL")
                testConfig.setAttribute("server.HEGEL_DeviceServer_1", "status", "on")

                testConfig.set("device.HEGEL_Conveyor_0", Hash())
                #testConfig.set("device.HEGEL_Conveyor_0.description", Schema())
                #testConfig.set("device.HEGEL_Conveyor_0.configuration", Hash())
                testConfig.setAttribute("device.HEGEL_Conveyor_0", "host", "HEGEL")
                testConfig.setAttribute("device.HEGEL_Conveyor_0", "classId", "Conveyor")
                testConfig.setAttribute("device.HEGEL_Conveyor_0", "serverId", "HEGEL_DeviceServer_0")
                testConfig.setAttribute("device.HEGEL_Conveyor_0", "status", "ok")
                
                #print ""
                #print testConfig
                #print ""
                Manager().handleRuntimeSystemDescription(testConfig)
            
        
            # Invalidate variables            
            self.__bodySize = self.__headerSize = 0
            self.__headerBytes = self.__bodyBytes = bytearray()


    def onDisplayServerError(self, socketError):
        print "onDisplayServerError", self.__tcpSocket.errorString()


    def onConnected(self):
        print "Connected to server"
        pass
        
        
    def onDisconnected(self):
        print "Disconnected from server"
        pass


    def onKillDeviceInstance(self, devSrvInsId, devInsId):
        header = Hash()
        header.set("type", "killDeviceInstance")
        header.set("devSrvInsId", str(devSrvInsId))
        header.set("devInsId", str(devInsId));
        self._tcpWriteHashHash(header, Hash())


    def onKillDeviceServerInstance(self, devSrvInsId):
        header = Hash()
        header.set("type", "killDeviceServerInstance")
        header.set("instanceId", str(devSrvInsId))
        self._tcpWriteHashHash(header, Hash())


    def onRefreshInstance(self, instanceId):
        header = Hash()
        header.set("type", "refreshInstance")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, Hash())
        
        
    def onReconfigure(self, instanceId, attributeId, attributeValue):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("instanceId", str(instanceId))
        body = Hash()
        body.set(str(attributeId), attributeValue)
        self._tcpWriteHashHash(header, body)


    def onReconfigureAsHash(self, instanceId, body):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, body)


    def onInitDevice(self, instanceId, config):
        header = Hash()
        header.set("type", "initDevice")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, config)


    def onCreateNewDeviceClassPlugin(self, devSrvInsId, classId, newClassId):
        header = Hash("type", "createNewDeviceClassPlugin")
        body = Hash()
        body.set("devSrvInsId", str(devSrvInsId))
        body.set("classId", str(classId))
        body.set("newClassId", str(newClassId))
        self._tcpWriteHashHash(header, body)


    def onSlotCommand(self, instanceId, info):
        header = Hash()
        header.set("type", "slotCommand")
        header.set("instanceId", str(instanceId))
        
        body = Hash()
        name = info.get(QString('name'))
        if name is None:
            name = info.get('name')
        body.set("name", str(name))
        
        args = info.get(QString('args'))
        if name is None:
            args = info.get('args')
        if args:
            i = 0
            for arg in args:
                i = i+1
                argName = str("a%s" % i)
                body.set(argName, str(arg))
        self._tcpWriteHashHash(header, body)


    def onNewVisibleDeviceInstance(self, instanceId):
        header = Hash()
        header.set("type", "newVisibleDeviceInstance")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, Hash())


    def onRemoveVisibleDeviceInstance(self, instanceId):
        header = Hash()
        header.set("type", "removeVisibleDeviceInstance")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, Hash())


### private functions ###
    def _sendLoginInformation(self, username, password):
        header = Hash("type", "login")
        body = Hash()
        body.set("username", str(username))
        body.set("password", str(password))
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

