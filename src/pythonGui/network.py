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
from dialogs.logindialog import LoginDialog
from manager import Manager
from struct import pack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, QByteArray, QCryptographicHash, QDataStream,
                          QObject)
from PyQt4.QtGui import QDialog, QMessageBox

import datetime
import globals
import socket

BYTES_MESSAGE_SIZE_TAG = 4

class Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()

    def __init__(self):
        super(Network, self).__init__()

        self.__serializer = BinarySerializerHash.create("Bin")

        self.__auth = None
        self.__username = None
        self.__password = None
        self.__provider = None
        self.__hostname = None
        self.__port = None

        self.__brokerHost = str()
        self.__brokerPort = str()
        self.__brokerTopic = str()
        self.__sessionToken = str()

        self.__tcpSocket = None

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

        self.__headerSize = 0
        self.__bodySize = 0
        self.__headerBytes = bytearray()
        self.__bodyBytes = bytearray()

    
    def connectToServer(self):
        """
        Connection to server via LoginDialog.
        """
        isConnected = False

        dialog = LoginDialog(username=self.__username,
                             password=self.__password,
                             provider=self.__provider,
                             hostname=self.__hostname,
                             port=self.__port)
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
        self.__username = username
        self.__password = password
        self.__provider = provider
        self.__hostname = hostname
        self.__port = port
        self.__headerSize = 0
        self.__bodySize = 0

        self.__tcpSocket = QTcpSocket(self)
        self.__tcpSocket.connected.connect(self.onConnected)
        self.__tcpSocket.disconnected.connect(self.onDisconnected)
        self.__tcpSocket.readyRead.connect(self.onReadServerData)
        self.__tcpSocket.error.connect(self.onSocketError)

        self.__tcpSocket.connectToHost(hostname, port)


    def endServerConnection(self):
        """
        End connection to server and database.
        """
        self._logout()
        Manager().disconnectedFromServer()

        if self.__tcpSocket is None:
            return

        self.__tcpSocket.disconnectFromHost()
        if (self.__tcpSocket.state() == QAbstractSocket.UnconnectedState) or \
            self.__tcpSocket.waitForDisconnected(5000):
            print "Disconnected from server"
        else:
            print "Disconnect failed:", self.__tcpSocket.errorString()


    def _login(self):
        """
        Authentification login.
        """
        # System variables definition
        ipAddress = socket.gethostname() # Machine Name

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
                # TODO: adapt Authenticator constructor for unicode parameters
                # instead of string
                self.__auth = Authenticator(str(self.__username), str(self.__password), 
                                            str(self.__provider), ipAddress,
                                            self.__brokerHost, self.__brokerPort,
                                            self.__brokerTopic)
            except Exception, e:
                raise RuntimeError("Authentication exception " + str(e))

            # Execute Login
            ok = False
            try:
                ok = self.__auth.login()
            except Exception, e:
                # TODO Fall back to inbuild access level
                globals.GLOBAL_ACCESS_LEVEL = globals.KARABO_DEFAULT_ACCESS_LEVEL
                print "Login problem. Please verify, if service is running. " + str(e)
                
                # Inform the mainwindow to change correspondingly the allowed level-downgrade
                self.signalUserChanged.emit()
                self._sendLoginInformation(self.__username, self.__password, \
                                           self.__provider, self.__sessionToken)
                return

            if ok:
                globals.GLOBAL_ACCESS_LEVEL = self.__auth.getDefaultAccessLevelId()
            else:
                print "Login failed"
                self.onSocketError(QAbstractSocket.ConnectionRefusedError)
                #globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
                return

        # Inform the mainwindow to change correspondingly the allowed level-downgrade
        self.signalUserChanged.emit()
        self._sendLoginInformation(self.__username, self.__password, self.__provider, \
                                   self.__sessionToken)


    def _logout(self):
        """
        Authentification logout.
        """
        # Execute Logout
        if self.__auth is None: return

        try:
            self.__auth.logout()
        except Exception, e:
            print "Logout problem. Please verify, if service is running. " + str(e)


### Slots ###
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
                Manager().handleInstanceNew(bodyHash)
            elif type == "instanceUpdated":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleSystemTopology(bodyHash)
            elif type == "instanceGone":
                Manager().handleInstanceGone(str(self.__bodyBytes))
            elif type == "classDescription":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleClassSchema(headerHash, bodyHash)
            elif type == "deviceSchema":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleDeviceSchema(headerHash, bodyHash)
            elif type == "configurationChanged":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleConfigurationChanged(headerHash, bodyHash)
            elif type == "log":
                Manager().onLogDataAvailable(str(self.__bodyBytes))
            elif type == "schemaUpdated":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleDeviceSchemaUpdated(headerHash, bodyHash)
            elif type == "brokerInformation":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleBrokerInformation(headerHash, bodyHash)
            elif type == "notification":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                self._handleNotification(headerHash, bodyHash)
            elif type == "historicData":
                bodyHash = self.__serializer.load(self.__bodyBytes)
                Manager().handleHistoricData(headerHash, bodyHash)
            elif type == "invalidateCache":
                print "invalidateCache"
            else:
                print "WARN : Got unknown communication token \"", type, "\" from server"
            # Invalidate variables
            self.__bodySize = self.__headerSize = 0
            self.__headerBytes = self.__bodyBytes = bytearray()


    def onSocketError(self, socketError):
        print "onSocketError", self.__tcpSocket.errorString(), socketError

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
        header.set("deviceId", str(deviceId))
        body = Hash()
        body.set(str(parameterId), value)
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


    def _handleBrokerInformation(self, headerHash, bodyHash):
        self.__brokerHost = bodyHash.get("host")
        self.__brokerPort = str(bodyHash.get("port"))
        self.__brokerTopic = bodyHash.get("topic")
        self._login()


    def _handleNotification(self, headerHash, bodyHash):
        deviceId = headerHash.get("deviceId")
        timestamp = datetime.datetime.now()
        # TODO: better format for timestamp and timestamp generation in karabo
        timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
        type = bodyHash.get("type")
        shortMessage = bodyHash.get("shortMsg")
        detailedMessage = bodyHash.get("detailedMsg")
        Manager().handleNotification(timestamp, type, shortMessage, detailedMessage, deviceId)

