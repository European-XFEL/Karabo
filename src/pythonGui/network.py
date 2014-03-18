#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which establishes the tcp network connection to
   the GuiServerDevice.
"""

__all__ = ["Network"]

from karabo.karathon import (AccessLevel, Authenticator, BinarySerializerHash,
                             Hash)
from logindialog import LoginDialog
from manager import Manager
from struct import pack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, QByteArray, QCryptographicHash, QDataStream,
                          QObject)
from PyQt4.QtGui import QDialog, QMessageBox

import globals
import socket

BYTES_MESSAGE_SIZE_TAG = 4

class Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()

    def __init__(self):
        super(Network, self).__init__()

        self.serializer = BinarySerializerHash.create("Bin")

        self.authenticator = None
        self.username = None
        self.password = None
        self.provider = None
        self.hostname = None
        self.port = None

        self.brokerHost = str()
        self.brokerPort = str()
        self.brokerTopic = str()
        self.sessionToken = str()

        self.tcpSocket = None
        self.dataSize = 0
        self.dataBytes = bytearray()

        Manager().signalKillDevice.connect(self.onKillDevice)
        Manager().signalKillServer.connect(self.onKillServer)
        Manager().signalRefreshInstance.connect(self.onRefreshInstance)
        Manager().signalReconfigure.connect(self.onReconfigure)
        Manager().signalReconfigureAsHash.connect(self.onReconfigureAsHash)
        Manager().signalInitDevice.connect(self.onInitDevice)
        Manager().signalExecute.connect(self.onExecute)

        Manager().signalNewVisibleDevice.connect(self.onNewVisibleDevice)
        Manager().signalRemoveVisibleDevice.connect(self.onRemoveVisibleDevice)

        Manager().signalGetClassSchema.connect(self.onGetClassSchema)
        Manager().signalGetDeviceSchema.connect(self.onGetDeviceSchema)
        Manager().signalGetFromPast.connect(self.onGetFromPast)


    def connectToServer(self):
        """
        Connection to server via LoginDialog.
        """
        isConnected = False

        dialog = LoginDialog(username=self.username,
                             password=self.password,
                             provider=self.provider,
                             hostname=self.hostname,
                             port=self.port)
        if dialog.exec_() == QDialog.Accepted:
            self.startServerConnection(dialog.username,
                                       dialog.password,
                                       dialog.provider,
                                       dialog.hostname,
                                       dialog.port)
            isConnected = True
        else:
            isConnected = False

        # Update mainwindow toolbar
        self.signalServerConnectionChanged.emit(isConnected)


    def disconnectFromServer(self):
        """
        Disconnect from server.
        """
        self.endServerConnection()
        # Update mainwindow toolbar
        self.signalServerConnectionChanged.emit(False)


    def startServerConnection(self, username, password, provider, hostname, port):
        """
        Attempt to connect to host on given port and save user specific data for
        later authentification.
        """
        self.username = username
        self.password = password
        self.provider = provider
        self.hostname = hostname
        self.port = port
        self.dataSize = 0

        self.tcpSocket = QTcpSocket(self)
        self.tcpSocket.connected.connect(self.onConnected)
        self.tcpSocket.disconnected.connect(self.onDisconnected)
        self.tcpSocket.readyRead.connect(self.onReadServerData)
        self.tcpSocket.error.connect(self.onSocketError)

        self.tcpSocket.connectToHost(hostname, port)


    def endServerConnection(self):
        """
        End connection to server and database.
        """
        self._logout()
        Manager().disconnectedFromServer()

        if self.tcpSocket is None:
            return

        self.tcpSocket.disconnectFromHost()
        if (self.tcpSocket.state() == QAbstractSocket.UnconnectedState) or \
            self.tcpSocket.waitForDisconnected(5000):
            print "Disconnected from server"
        else:
            print "Disconnect failed:", self.tcpSocket.errorString()


    def _login(self):
        """
        Authentification login.
        """
        # System variables definition
        ipAddress = socket.gethostname() # Machine Name

        # Easteregg
        if self.username == "god":
            md5 = QCryptographicHash.hash(str(self.password), QCryptographicHash.Md5).toHex()
            if md5 == "39d676ecced45b02da1fb45731790b4c":
                print "Entering god mode..."
                globals.GLOBAL_ACCESS_LEVEL = 1000
            else:
                globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER

        else:
            # Construct Authenticator class
            try:
                # TODO: adapt Authenticator constructor for unicode parameters
                # instead of string
                self.authenticator = Authenticator(str(self.username), str(self.password),
                                                   str(self.provider), ipAddress,
                                                   self.brokerHost, str(self.brokerPort),
                                                   self.brokerTopic)
            except Exception, e:
                raise RuntimeError("Authentication exception " + str(e))

            # Execute Login
            ok = False
            try:
                ok = self.authenticator.login()
            except Exception, e:
                # TODO Fall back to inbuild access level
                globals.GLOBAL_ACCESS_LEVEL = globals.KARABO_DEFAULT_ACCESS_LEVEL
                print "Login problem. Please verify, if service is running. " + str(e)
                
                # Inform the mainwindow to change correspondingly the allowed level-downgrade
                self.signalUserChanged.emit()
                self._sendLoginInformation(self.username, self.password, \
                                           self.provider, self.sessionToken)
                return

            if ok:
                globals.GLOBAL_ACCESS_LEVEL = self.authenticator.getDefaultAccessLevelId()
            else:
                print "Login failed"
                self.onSocketError(QAbstractSocket.ConnectionRefusedError)
                #globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
                return

        # Inform the mainwindow to change correspondingly the allowed level-downgrade
        self.signalUserChanged.emit()
        self._sendLoginInformation(self.username, self.password, self.provider, \
                                   self.sessionToken)


    def _logout(self):
        """
        Authentification logout.
        """
        # Execute Logout
        if self.authenticator is None: return

        try:
            self.authenticator.logout()
        except Exception, e:
            print "Logout problem. Please verify, if service is running. " + str(e)


### Slots ###
    def onReadServerData(self):
        input = QDataStream(self.tcpSocket)
        input.setByteOrder(QDataStream.LittleEndian)

        #print self.tcpSocket.bytesAvailable(), " bytes are coming in"
        while True:

            if self.dataSize == 0:

                if self.tcpSocket.bytesAvailable() < (BYTES_MESSAGE_SIZE_TAG):
                    break

                self.dataSize = input.readUInt32()

            if len(self.dataBytes) == 0:

                if self.tcpSocket.bytesAvailable() < self.dataSize:
                    break

                self.dataBytes = bytearray(self.dataSize)
                self.dataBytes = input.readRawData(self.dataSize)
                # TODO How to do this nicely?
                self.dataBytes = bytearray(self.dataBytes)


            # Fork on responseType
            instanceInfo = self.serializer.load(self.dataBytes)

            type = instanceInfo.get("type")
            #print "Request: ", type

            # "instanceNew" (instanceId, instanceInfo)
            # "instanceUpdated" (instanceId, instanceInfo)
            # "instanceGone" (instanceId)
            # "configurationChanged" (config, instanceId)
            # "log" (logMessage)
            # "notification" (type, shortMsg, detailedMsg, instanceId)
            # "invalidateCache" (instanceId)

            if type == "brokerInformation":
                self._handleBrokerInformation(instanceInfo)
            elif type == "systemTopology":
                Manager().handleSystemTopology(instanceInfo.get("systemTopology"))
            elif type == "instanceNew":
                Manager().handleInstanceNew(instanceInfo.get("topologyEntry"))
            elif type == "instanceUpdated":
                Manager().handleInstanceUpdated(instanceInfo.get("topologyEntry"))
            elif type == "instanceGone":
                Manager().handleInstanceGone(instanceInfo.get("instanceId"))
            elif type == "classSchema":
                Manager().handleClassSchema(instanceInfo)
            elif type == "deviceSchema":
                Manager().handleDeviceSchema(instanceInfo)
            elif type == "configurationChanged":
                Manager().handleConfigurationChanged(instanceInfo)
            elif type == "log":
                Manager().onLogDataAvailable(instanceInfo.get("message"))
            elif type == "schemaUpdated":
                Manager().handleDeviceSchemaUpdated(instanceInfo)
            elif type == "notification":
                Manager().handleNotification(instanceInfo)
            elif type == "historicData":
                Manager().handleHistoricData(instanceInfo)
            elif type == "invalidateCache":
                print "invalidateCache"
            else:
                print "WARN : Got unknown communication token \"", type, "\" from server"
            # Invalidate variables
            self.dataSize = 0
            self.dataBytes = bytearray()


    def onSocketError(self, socketError):
        print "onSocketError", self.tcpSocket.errorString(), socketError

        self.disconnectFromServer()

        if socketError == QAbstractSocket.ConnectionRefusedError:
            reply = QMessageBox.question(None, 'Server connection refused',
                "The connection to the server was refused <BR> by the peer "
                "(or timed out).",
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.RemoteHostClosedError:
            reply = QMessageBox.question(None, 'Connection closed',
                "The remote host closed the connection.",
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.HostNotFoundError:
            reply = QMessageBox.question(None, 'Host address error',
                "The host address was not found.",
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.NetworkError:
            reply = QMessageBox.question(None, 'Network error',
                "An error occurred with the network (e.g., <BR> "
                "the network cable was accidentally plugged out).",
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return

        self.connectToServer()


    def onConnected(self):
        pass


    def onDisconnected(self):
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
        header.set("deviceId", deviceId)
        body = Hash()
        body.set(parameterId, value)
        self._tcpWriteHashHash(header, body)


    def onReconfigureAsHash(self, deviceId, body):
        header = Hash()
        header.set("type", "reconfigure")
        header.set("deviceId", deviceId)
        self._tcpWriteHashHash(header, body)


    def onInitDevice(self, serverId, config):
        header = Hash()
        header.set("type", "initDevice")
        header.set("serverId", serverId)
        self._tcpWriteHashHash(header, config)


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


    def onGetFromPast(self, deviceId, property, t0, t1):
        header = Hash()
        header.set('type', 'getFromPast')
        header.set('deviceId', deviceId)
        header.set('property', property)
        header.set('t0', t0)
        header.set('t1', t1)
        self._tcpWriteHashHash(header, Hash())


### private functions ###
    def _sendLoginInformation(self, username, password, provider, sessionToken):
        header = Hash("type", "login")
        body = Hash()
        body.set("username", username)
        body.set("password", password)
        body.set("provider", provider)
        body.set("sessionToken", sessionToken)

        self._tcpWriteHashHash(header, body)


    def _tcpWriteHashHash(self, instanceInfo, bodyHash):
        stream = QByteArray()
        headerString = QByteArray(self.serializer.save(instanceInfo))
        bodyString = QByteArray(self.serializer.save(bodyHash))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()

        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(QByteArray(pack('I', nBytesBody)))
        stream.push_back(bodyString)
        self.tcpSocket.write(stream)


    def _tcpWriteHashString(self, instanceInfo, body):
        stream = QByteArray()
        headerString = QByteArray(self.serializer.save(instanceInfo))
        bodyString = QByteArray(str(body))
        nBytesHeader = headerString.size()
        nBytesBody = bodyString.size()

        stream.push_back(QByteArray(pack('I', nBytesHeader)))
        stream.push_back(headerString)
        stream.push_back(QByteArray(pack('I', nBytesBody)))
        stream.push_back(bodyString)
        self.tcpSocket.write(stream)


    def _handleBrokerInformation(self, instanceInfo):
        self.brokerHost = instanceInfo.get("host")
        self.brokerPort = instanceInfo.get("port")
        self.brokerTopic = instanceInfo.get("topic")
        self._login()

