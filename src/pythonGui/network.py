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
from struct import pack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import pyqtSignal, QByteArray, QCryptographicHash, QObject
from PyQt4.QtGui import QDialog, QMessageBox
from karabo.authenticator import Authenticator
from karabo.hash import Hash, BinaryParser, BinaryWriter
from enums import AccessLevel

import globals
import socket
from struct import unpack


class _Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()
    signalReceivedData = pyqtSignal(object)

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
        
        self.requestQueue = [ ]


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
        self.signalServerConnectionChanged.emit(False)
        self.requestQueue = []

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
                self.authenticator = Authenticator(
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
                globals.GLOBAL_ACCESS_LEVEL = \
                    self.authenticator.defaultAccessLevelId
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
        dataSize, = unpack(b"I", (yield 4))
        dataBytes = yield dataSize

        h = BinaryParser().read(dataBytes)
        self.signalReceivedData.emit(h)


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
        """ connect states wether we should connect to or disconnect from
        the server. """
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


    def onConnected(self):
        # If some requests got piled up, because of no server connection,
        # now these get handled
        for r in self.requestQueue:
            self._tcpWriteHash(r)
        self.requestQueue = []


    def onDisconnected(self):
        pass


    def onKillDevice(self, deviceId):
        h = Hash("type", "killDevice")
        h.set("deviceId", deviceId);
        self._tcpWriteHash(h)


    def onKillServer(self, serverId):
        h = Hash("type", "killServer")
        h.set("serverId", serverId)
        self._tcpWriteHash(h)


    def onRefreshInstance(self, configuration):
        h = Hash("type", "refreshInstance")
        h.set("deviceId", configuration.id)
        self._tcpWriteHash(h)


    def onReconfigure(self, changes):
        """ set values in a device

        changes is a list of pairs (box, value). They all must belong to
        the same device."""
        h = Hash()
        h["type"] = "reconfigure"
        id = changes[0][0].configuration.id
        h["deviceId"] = id
        conf = Hash()
        for box, value in changes:
            assert id == box.configuration.id
            conf[".".join(box.path)] = box.descriptor.cast(value)
        h["configuration"] = conf
        self._tcpWriteHash(h)


    def onInitDevice(self, serverId, config):
        h = Hash("type", "initDevice")
        h.set("serverId", serverId)
        h.set("configuration", config)
        self._tcpWriteHash(h)


    def onExecute(self, box, *args):
        h = Hash("type", "execute")
        h.set("deviceId", box.configuration.id)
        h.set("command", box.path[-1])

        if args is not None:
            i = 0
            for arg in args:
                i = i+1
                argName = "a{}".format(i)
                h.set(argName, arg)

        self._tcpWriteHash(h)


    def onNewVisibleDevice(self, deviceId):
        h = Hash("type", "newVisibleDevice")
        h.set("deviceId", deviceId)
        self._tcpWriteHash(h)


    def onRemoveVisibleDevice(self, deviceId):
        h = Hash("type", "removeVisibleDevice")
        h.set("deviceId", deviceId)
        self._tcpWriteHash(h)


    def onGetClassSchema(self, serverId, classId):
        h = Hash("type", "getClassSchema")
        h.set("serverId", serverId)
        h.set("classId", classId)
        self._tcpWriteHash(h)


    def onGetDeviceSchema(self, deviceId):
        h = Hash("type", "getDeviceSchema")
        h.set("deviceId", deviceId)
        self._tcpWriteHash(h)


    def onGetFromPast(self, box, t0, t1, maxNumData):
        h = Hash("type", "getFromPast")
        h.set("deviceId", box.configuration.id)
        h.set("property", ".".join(box.path))
        h.set("t0", t0)
        h.set("t1", t1)
        h.set("maxNumData", maxNumData)
        self._tcpWriteHash(h)


    def onError(self, error):
        h = Hash("type", "error", "traceback", error)
        self._tcpWriteHash(h)


### private functions ###
    def _tcpWriteHash(self, h):
        # There might be a connect to server in progress, but without success
        if self.tcpSocket is None or \
           self.tcpSocket.state() == QAbstractSocket.HostLookupState or \
           self.tcpSocket.state() == QAbstractSocket.ConnectingState:
            # Save request for connection established
            self.requestQueue.append(h)
            return

        stream = QByteArray()
        writer = BinaryWriter()
        dataBytes = writer.write(h)
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


    def _handleBrokerInformation(self, h):
        self.brokerHost = h.get("host")
        self.brokerPort = h.get("port")
        self.brokerTopic = h.get("topic")
        self._login()


network = _Network()

def Network():
    return network
