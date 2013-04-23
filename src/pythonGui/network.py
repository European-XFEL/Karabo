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

class Network(QObject):
           
    def __init__(self):
        super(Network, self).__init__()
        
        self.__serializer = FormatHash.create("Xml", Hash("printDataType", 1))
        
        self.__binarySerializer = FormatHash.create("Bin", Hash())
                       
        self.__tcpSocket = QTcpSocket(self)
        self.__tcpSocket.connected.connect(self.onConnected)
        self.__tcpSocket.disconnected.connect(self.onDisconnected)
        self.__tcpSocket.readyRead.connect(self.onReadServerData)
        self.__tcpSocket.error.connect(self.onDisplayServerError)
        
        Manager().notifier.signalKillDeviceInstance.connect(self.onKillDeviceInstance)
        Manager().notifier.signalKillDeviceServerInstance.connect(self.onKillDeviceServerInstance)
        Manager().notifier.signalRefreshInstance.connect(self.onRefreshInstance)
        Manager().notifier.signalReconfigure.connect(self.onReconfigure)
        Manager().notifier.signalReconfigureAsHash.connect(self.onReconfigureAsHash)
        Manager().notifier.signalInitDevice.connect(self.onInitDevice)
        Manager().notifier.signalSlotCommand.connect(self.onSlotCommand)
        
        Manager().notifier.signalNewVisibleDeviceInstance.connect(self.onNewVisibleDeviceInstance)
        Manager().notifier.signalRemoveVisibleDeviceInstance.connect(self.onRemoveVisibleDeviceInstance)
        
        Manager().notifier.signalCreateNewDeviceClassPlugin.connect(self.onCreateNewDeviceClassPlugin)
        
        self.__totalSize = 0
        self.__headerSize = 0
        
        
    def onStartConnection(self):
        dialog = LoginDialog()
        if dialog.exec_() == QDialog.Accepted :
            # test request to server
            self.__totalSize = 0
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


### Slots ###
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
        body.setFromPath(str(attributeId), attributeValue)
        self._tcpWriteHashHash(header, body)


    def onReconfigureAsHash(self, instanceId, body):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, body)


    def _sendLoginInformation(self, username, password):
        header = Hash("type", "login")
        body = Hash()
        body.set("username", str(username))
        body.set("password", str(password))
        self._tcpWriteHashHash(header, body)
        
        
    def onInitDevice(self, instanceId, config):
        header = Hash()
        header.set("type", "initDevice")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashHash(header, config)


    def onCreateNewDeviceClassPlugin(self, devSrvInsId, devClaId, newDevClaId):
        header = Hash("type", "createNewDeviceClassPlugin")
        body = Hash()
        body.set("devSrvInsId", str(devSrvInsId))
        body.set("devClaId", str(devClaId))
        body.set("newDevClaId", str(newDevClaId))
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
        self._tcpWriteHashString(header, Hash())


    def onRemoveVisibleDeviceInstance(self, instanceId):
        header = Hash()
        header.set("type", "removeVisibleDeviceInstance")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashString(header, Hash())


    def _tcpWriteHashHash(self, headerHash, bodyHash):
        stream = QByteArray()
        headerString = QByteArray(self.__serializer.serialize(headerHash))
        bodyString = QByteArray(self.__serializer.serialize(bodyHash))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()
                
        nBytesTotal = nBytesHeader + nBytesBody
        stream.push_back(QByteArray(pack('I', nBytesTotal)))
        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(bodyString)
        self.__tcpSocket.write(stream)
        
        
    def _tcpWriteHashString(self, headerHash, body):
        stream = QByteArray()
        headerString = QByteArray(self.__serializer.serialize(headerHash))
        bodyString = QByteArray(str(body))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()
        nBytesTotal = nBytesHeader + nBytesBody
        
        stream.push_back(QByteArray(pack('I', nBytesTotal)))
        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(bodyString)
        self.__tcpSocket.write(stream)
        
                
    def onReadServerData(self):
        input = QDataStream(self.__tcpSocket)
        input.setByteOrder(QDataStream.LittleEndian)

        #print self.__tcpSocket.bytesAvailable(), " bytes are coming in"
        while True:

            if self.__totalSize == 0 :
            
                if self.__tcpSocket.bytesAvailable() < (BYTES_TOTAL_SIZE_TAG + BYTES_HEADER_SIZE_TAG) :
                    break

                self.__totalSize = input.readUInt32()
                self.__headerSize = input.readUInt32()
          
            if self.__tcpSocket.bytesAvailable() < self.__totalSize :
                break

            # Fill header
            headerString = QString()
            headerString.resize(self.__headerSize)
            headerString = input.readRawData(self.__headerSize)
            
            # Fill body
            bodyString = QString()
            bodySize = self.__totalSize - self.__headerSize
            bodyString.resize(bodySize)
            bodyString = input.readRawData(bodySize)
            
            # Fork on responseType
            headerHash = self.__serializer.unserialize(str(headerString))
            
            type = headerHash.get("type")
            #print "Request: ", type
            
            # "instanceUpdated" (instanceId, instanceInfo)
            # "instanceGone" (instanceId)
            # "configurationChange" (config, instanceId)
            # "log" (logMessage)
            # "notify" (instanceId, type, text)
            # "invalidateCache" (instanceId)
            
            if type == "change": 
                bodyHash = self.__binarySerializer.unserialize(str(bodyString))
                self._handleChange(headerHash, bodyHash)
            elif type == "log": 
                self._handleLog(bodyString)
            elif type == "error":
                self._handleError(bodyString)
            elif type == "warning":
                self._handleWarning(bodyString)
            elif type == "alarm":
                self._handleAlarm(bodyString)
            elif type == "currentInstances":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleInstanceIds(bodyHash)
                #self._tcpWrite(headerHash, bodyHash)
            elif type == "newNode":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleNewNode(bodyHash)
            elif type == "newDeviceServerInstance":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleNewDeviceServerInstance(bodyHash)
            elif type == "newDeviceClass":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleNewDeviceClass(bodyHash)
            elif type == "newDeviceInstance":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleNewDeviceInstanceOnRunning(bodyHash)
            elif type == "updateDeviceServerInstance":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleUpdateDeviceServerInstance(bodyHash)
            elif type == "updateDeviceInstance":
                bodyHash = self.__serializer.unserialize(str(bodyString))
                self._handleUpdateDeviceInstance(bodyHash)
            elif type == "schemaUpdated":
                self._handleSchemaUpdated(headerHash, bodyString)
                    
            self.__totalSize = self.__headerSize = 0


    def onDisplayServerError(self, socketError):
        print "onDisplayServerError", self.__tcpSocket.errorString()


    def onConnected(self):
        #print "Connected to server"
        pass
        
        
    def onDisconnected(self):
        #print "Disconnected from server"
        pass


    def _sendRefreshRequest(self, instanceId):
        header = Hash()
        header.set("type", "refreshInstance")
        header.set("instanceId", str(instanceId))
        self._tcpWriteHashString(header, Hash())


    def _handleChange(self, headerHash, bodyHash):
        instanceId = headerHash.get("instanceId")
        #classId = headerHash.get("classId")
        Manager().onConfigChanged(instanceId, bodyHash)


    def _handleLog(self, logMessage):
        Manager().onLogDataAvailable(logMessage)


    def _handleError(self, errorMessage):
        Manager().onErrorDataAvailable(errorMessage)


    def _handleWarning(self, warningMessage):
        Manager().onWarningDataAvailable(warningMessage)
        
        
    def _handleAlarm(self, alarmMessage):
        Manager().onAlarmDataAvailable(alarmMessage)


    def _handleInstanceIds(self, body):
        root = body.get("Root")
        node = root.get("Node") # This reflects the "Node" DB-Table
        # Inform about new nodes
        for row in node: # Loop over all rows in the table
            self._handleNewNode(row)

        # Inform about new deviceServerInstances
        deviceServerInstance = root.get("DeviceServerInstance")
        for row in deviceServerInstance:
            self._handleNewDeviceServerInstance(row)
                    
        # Inform about new deviceClasses
        deviceClass = root.get("DeviceClass")
        for row in deviceClass:
            self._handleNewDeviceClass(row)
        
        # Inform about new deviceInstances
        deviceInstance = root.get("DeviceInstance")
        for row in deviceInstance:
            self._handleNewDeviceInstanceOnStartUp(row)
            
    
    def _handleNewNode(self, row):
        info = self._collectNodeInformation(row)
        Manager().onNewNode(info)
        
                    
    def _handleNewDeviceServerInstance(self, row):
         info = self._collectDeviceServerInstanceInformation(row)
         Manager().onNewDeviceServerInstance(info)
                 
        
    def _handleNewDeviceClass(self, row):
        info = self._collectDeviceClassInformation(row)
        Manager().onNewDeviceClass(info)
                        
        
    def _handleNewDeviceInstanceOnStartUp(self, row):
        info = self._collectDeviceInstanceInformation(row)
        Manager().onNewDeviceInstance(info)


    def _handleNewDeviceInstanceOnRunning(self, row):
        info = self._collectDeviceInstanceInformation(row)
        Manager().onNewDeviceInstance(info)
        Manager().onSelectNewDeviceInstance(info)


    def _handleUpdateDeviceServerInstance(self, row):
        info = self._collectDeviceServerInstanceInformation(row)
        Manager().onUpdateDeviceServerInstance(info)


    def _handleUpdateDeviceInstance(self, row):
        info = self._collectDeviceInstanceInformation(row)
        Manager().onUpdateDeviceInstance(info)


    def _handleSchemaUpdated(self, headerHash, schema):
        instanceId = headerHash.get("instanceId")
        #classId = headerHash.get("classId")
        Manager().onSchemaUpdated(instanceId, schema)


    def _collectNodeInformation(self, row):
        id = row.get("id")
        name = row.get("name")
        return dict(id=id, name=name)
    
    
    def _collectDeviceServerInstanceInformation(self, row):
        id = row.get("id")
        name = row.get("instanceId")
        status = row.get("status")
        nodId = row.get("nodId")
        return dict(id=id, name=name, status=status, refId=nodId)
    
        
    def _collectDeviceClassInformation(self, row):
        id = row.get("id")
        name = row.get("name")
        schema = row.get("schema")
        devSerInsId = row.get("devSerInsId")
        return dict(id=id, name=name, schema=schema, refId=devSerInsId)
    
    
    def _collectDeviceInstanceInformation(self, row):
        id = row.get("id")
        name = row.get("instanceId")
        devClaId = row.get("devClaId")
        schema = row.get("schema")
        return dict(id=id, name=name, refId=devClaId, schema=schema)

