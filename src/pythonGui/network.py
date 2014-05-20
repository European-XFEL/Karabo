from __future__ import unicode_literals
#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which establishes the tcp network connection to
   the GuiServerDevice.
   
   The network class is a singleton.
"""

__all__ = ["Network"]

from dialogs.logindialog import LoginDialog
from manager import Manager
from struct import pack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, QByteArray, QCryptographicHash,
                          QObject, QMutex, QMutexLocker)
from PyQt4.QtGui import QDialog, QMessageBox
from karabo.py_authenticator import PyAuthenticator
from karabo.hash import Hash, BinaryParser, BinaryWriter
from enums import AccessLevel

import globals
from Queue import Queue
import socket
from struct import unpack


class _Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()

    def __init__(self):
        super(_Network, self).__init__()

        self.authenticator = None
        self.username = None
        self.password = None
        self.provider = None
        self.hostname = None
        self.port = None

        self.brokerHost = ""
        self.brokerPort = ""
        self.brokerTopic = ""
        self.sessionToken = ""

        self.tcpSocket = None
        
        self.requestQueue = Queue()
        self.requestMutex = QMutex()

        Manager().signalKillDevice.connect(self.onKillDevice)
        Manager().signalKillServer.connect(self.onKillServer)
        Manager().signalRefreshInstance.connect(self.onRefreshInstance)
        Manager().signalReconfigure.connect(self.onReconfigure)
        Manager().signalInitDevice.connect(self.onInitDevice)
        Manager().signalExecute.connect(self.onExecute)

        Manager().signalNewVisibleDevice.connect(self.onNewVisibleDevice)
        Manager().signalRemoveVisibleDevice.connect(self.onRemoveVisibleDevice)

        Manager().signalGetClassSchema.connect(self.onGetClassSchema)
        Manager().signalGetDeviceSchema.connect(self.onGetDeviceSchema)
        Manager().signalGetFromPast.connect(self.onGetFromPast)
        
        self.signalServerConnectionChanged.connect(Manager().systemTopology.onServerConnectionChanged)
        self.signalServerConnectionChanged.connect(Manager().projectTopology.onServerConnectionChanged)


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

            # If some requests got pilled up, because of no server connection,
            # now these get handled
            with QMutexLocker(self.requestMutex):
                while not self.requestQueue.empty():
                    self._tcpWriteHash(self.requestQueue.get())
            
            isConnected = True
        else:
            isConnected = False

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(isConnected)


    def disconnectFromServer(self):
        """
        Disconnect from server.
        """
        self.endServerConnection()
        # Update MainWindow toolbar
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
        self.runner = self.processInput()
        self.bytesNeeded = self.runner.next()
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
            return
        
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
                self.authenticator = PyAuthenticator(
                            self.username, self.password, self.provider, ipAddress,
                            self.brokerHost, self.brokerPort, self.brokerTopic)
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


    def onReadServerData(self):
        while self.tcpSocket.bytesAvailable() >= self.bytesNeeded:
            try:
                self.bytesNeeded = self.runner.send(self.tcpSocket.read(
                    self.bytesNeeded))
            except Exception as e:
                self.runner = self.processInput()
                self.bytesNeeded = self.runner.next()
                if not isinstance(e, StopIteration):
                    raise


    def processInput(self):
        parser = BinaryParser()

        dataSize, = unpack('I', (yield 4))
        dataBytes = yield dataSize

        instanceInfo = parser.read(dataBytes)

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
            Manager().handleSystemTopology(instanceInfo.get("topologyEntry"))
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
        elif type == "propertyHistory":
            Manager().handleHistoricData(instanceInfo)
        elif type == "invalidateCache":
            print "invalidateCache"
        else:
            print "WARN : Got unknown communication token \"", type, "\" from server"


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


    def onServerConnection(self, connect):
        """
        This slot is triggered from the MainWindow.
        
        If the user wants the connect the GUI client to the
        GuiServer \connect is True, else False.
        """
        if connect:
            self.connectToServer()
        else:
            self.disconnectFromServer()


    def onQuitApplication(self):
        """
        This slot is triggered from the MainWindow.
        
        The user wants to quit the application.
        So the network connection needs to be closed
        and the manager needs to be informed to close
        the database connection as well.
        """
        self.endServerConnection()
        Manager().closeDatabaseConnection()


    def onConnected(self):
        pass


    def onDisconnected(self):
        pass


    def onKillDevice(self, deviceId):
        instanceInfo = Hash("type", "killDevice")
        instanceInfo.set("deviceId", deviceId);
        self._tcpWriteHash(instanceInfo)


    def onKillServer(self, serverId):
        instanceInfo = Hash("type", "killServer")
        instanceInfo.set("serverId", serverId)
        self._tcpWriteHash(instanceInfo)


    def onRefreshInstance(self, configuration):
        instanceInfo = Hash("type", "refreshInstance")
        instanceInfo.set("deviceId", configuration.key)
        self._tcpWriteHash(instanceInfo)


    def onReconfigure(self, boxes):
        instanceInfo = Hash()
        instanceInfo["type"] = "reconfigure"
        instanceInfo["deviceId"] = boxes[0].configuration.key
        for b in boxes:
            instanceInfo["configuration." + ".".join(b.path)] = b.toHash()
        self._tcpWriteHash(instanceInfo)


    def onInitDevice(self, serverId, config):
        instanceInfo = Hash("type", "initDevice")
        instanceInfo.set("serverId", serverId)
        instanceInfo.set("configuration", config)
        self._tcpWriteHash(instanceInfo)


    def onExecute(self, deviceId, info):
        instanceInfo = Hash("type", "execute")
        instanceInfo.set("deviceId", deviceId)
        instanceInfo.set("command", info.get('command'))

        args = info.get('args')
        if args is not None:
            i = 0
            for arg in args:
                i = i+1
                argName = "a{}".format(i)
                instanceInfo.set(argName, arg)

        self._tcpWriteHash(instanceInfo)


    def onNewVisibleDevice(self, deviceId):
        instanceInfo = Hash("type", "newVisibleDevice")
        instanceInfo.set("deviceId", deviceId)
        self._tcpWriteHash(instanceInfo)


    def onRemoveVisibleDevice(self, deviceId):
        instanceInfo = Hash("type", "removeVisibleDevice")
        instanceInfo.set("deviceId", deviceId)
        self._tcpWriteHash(instanceInfo)


    def onGetClassSchema(self, serverId, classId):
        instanceInfo = Hash("type", "getClassSchema")
        instanceInfo.set("serverId", serverId)
        instanceInfo.set("classId", classId)
        self._tcpWriteHash(instanceInfo)


    def onGetDeviceSchema(self, deviceId):
        instanceInfo = Hash("type", "getDeviceSchema")
        instanceInfo.set("deviceId", deviceId)
        self._tcpWriteHash(instanceInfo)


    def onGetFromPast(self, box, t0, t1, maxNumData):
        instanceInfo = Hash("type", "getFromPast")
        instanceInfo.set("deviceId", box.configuration.key)
        instanceInfo.set("property", ".".join(box.path))
        instanceInfo.set("t0", t0)
        instanceInfo.set("t1", t1)
        instanceInfo.set("maxNumData", maxNumData)
        self._tcpWriteHash(instanceInfo)


### private functions ###
    def _tcpWriteHash(self, instanceInfo):
        if self.tcpSocket is None:
            # Save request for connection established
            self.requestQueue.put(instanceInfo)
            return

        stream = QByteArray()
        writer = BinaryWriter()
        dataBytes = writer.write(instanceInfo)
        stream.push_back(QByteArray(pack('I', len(dataBytes))))
        stream.push_back(dataBytes)
        self.tcpSocket.write(stream)


    def _sendLoginInformation(self, username, password, provider, sessionToken):
        loginInfo = Hash("type", "login")
        loginInfo.set("username", username)
        loginInfo.set("password", password)
        loginInfo.set("provider", provider)
        loginInfo.set("sessionToken", sessionToken)
        self._tcpWriteHash(loginInfo)


    def _handleBrokerInformation(self, instanceInfo):
        self.brokerHost = instanceInfo.get("host")
        self.brokerPort = instanceInfo.get("port")
        self.brokerTopic = instanceInfo.get("topic")
        self._login()


network = _Network()

def Network():
    return network
