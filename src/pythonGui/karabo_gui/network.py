
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

from karabo_gui.dialogs.logindialog import LoginDialog
from struct import pack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, pyqtSlot, QByteArray, QCoreApplication,
                          QCryptographicHash, QObject, QTimer)
from PyQt4.QtGui import QDialog, QMessageBox
from karabo.authenticator import Authenticator
from karabo.api_2 import Hash, BinaryParser, BinaryWriter, AccessLevel

import karabo_gui.globals as globals
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
        self.timer = QTimer(self)
        self.timer.setInterval(0)
        self.timer.timeout.connect(self.onReadServerData)
        
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
        # All panels need to be reseted and all projects closed
        self.signalServerConnectionChanged.emit(False)
        self.endServerConnection()


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
        self.bytesNeeded = next(self.runner)
        self.tcpSocket.disconnected.connect(self.onDisconnected)
        self.tcpSocket.readyRead.connect(self.timer.start)
        self.tcpSocket.error.connect(self.onSocketError)

        self.tcpSocket.connectToHost(hostname, port)


    def endServerConnection(self):
        """
        End connection to server and database.
        """
        self._logout()
        self.requestQueue = []

        if self.tcpSocket is None:
            return

        self.tcpSocket.disconnectFromHost()
        if (self.tcpSocket.state() == QAbstractSocket.UnconnectedState) or \
            self.tcpSocket.waitForDisconnected(5000):
            return
        
        print("Disconnect failed:", self.tcpSocket.errorString())


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
                print("Entering god mode...")
                globals.GLOBAL_ACCESS_LEVEL = AccessLevel.GOD
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
            except Exception as e:
                print("Authenticator not available.", str(e))

            # Execute Login
            ok = False
            try:
                ok = self.authenticator.login()
            except Exception as e:
                # TODO Fall back to inbuild access level
                globals.GLOBAL_ACCESS_LEVEL = globals.KARABO_DEFAULT_ACCESS_LEVEL
                print("Login problem. Please verify, if service is running. " + str(e))
                
                # Inform the mainwindow to change correspondingly the allowed level-downgrade
                self.signalUserChanged.emit()
                self._sendLoginInformation(self.username, self.password, \
                                           self.provider, self.sessionToken)
                return

            if ok:
                globals.GLOBAL_ACCESS_LEVEL = \
                    AccessLevel(self.authenticator.defaultAccessLevelId)
            else:
                print("Login failed")
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
        except Exception as e:
            print("Logout problem. Please verify, if service is running. " + str(e))

    @pyqtSlot()
    def onReadServerData(self):
        if self.isDataPending():
            try:
                self.bytesNeeded = self.runner.send(self.tcpSocket.read(
                    self.bytesNeeded))
            except Exception as e:
                self.runner = self.processInput()
                self.bytesNeeded = next(self.runner)
                if not isinstance(e, StopIteration):
                    raise
        else:
            self.timer.stop()

    def isDataPending(self):
        return self.tcpSocket.bytesAvailable() >= self.bytesNeeded


    def processInput(self):
        dataSize, = unpack(b"I", (yield 4))
        dataBytes = yield dataSize

        h = BinaryParser().read(dataBytes)
        self.signalReceivedData.emit(h)


    def onSocketError(self, socketError):
        print("onSocketError", self.tcpSocket.errorString(), socketError)

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


    def onGetDeviceConfiguration(self, configuration):
        h = Hash("type", "getDeviceConfiguration")
        h.set("deviceId", configuration.id)
        self._tcpWriteHash(h)


    def onReconfigure(self, changes):
        """ set values in a device

        changes is a list of pairs (box, value). They all must belong to
        the same device."""
        if not changes:
            return
        
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


    def onInitDevice(self, serverId, classId, deviceId, config):
        h = Hash("type", "initDevice")
        h.set("serverId", serverId)
        h.set("classId", classId)
        h.set("deviceId", deviceId)
        h.set("configuration", config)
        self._tcpWriteHash(h)


    def onExecute(self, box, *args):
        h = Hash("type", "execute")
        h.set("deviceId", box.configuration.id)
        h.set("command", box.path[-1])

        for i, arg in enumerate(args):
            h.set("a{}".format(i + 1), arg)

        self._tcpWriteHash(h)


    def onStartMonitoringDevice(self, deviceId):
        h = Hash("type", "startMonitoringDevice")
        h.set("deviceId", deviceId)
        self._tcpWriteHash(h)


    def onStopMonitoringDevice(self, deviceId):
        h = Hash("type", "stopMonitoringDevice")
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


    def onGetPropertyHistory(self, box, t0, t1, maxNumData):
        h = Hash("type", "getPropertyHistory")
        h.set("deviceId", box.configuration.id)
        h.set("property", ".".join(box.path))
        h.set("t0", t0)
        h.set("t1", t1)
        h.set("maxNumData", maxNumData)
        self._tcpWriteHash(h)

    
    def onGetAvailableProjects(self):
        h = Hash("type", "getAvailableProjects")
        self._tcpWriteHash(h)


    def onNewProject(self, fileName, data):
        h = Hash("type", "newProject")
        h.set("author", self.username)
        h.set("name", fileName)
        h.set("data", data)
        self._tcpWriteHash(h)


    def onSaveProject(self, filename, data):
        h = Hash("type", "saveProject")
        h.set("user", self.username)
        h.set("name", filename)
        h.set("data", data)
        self._tcpWriteHash(h)


    def onLoadProject(self, filename):
        h = Hash("type", "loadProject")
        h.set("user", self.username)
        h.set("name", filename)
        self._tcpWriteHash(h)


    def onCloseProject(self, filename):
        h = Hash("type", "closeProject")
        h.set("user", self.username)
        h.set("name", filename)
        self._tcpWriteHash(h)


    def onSubscribeToOutput(self, box, subscribe):
        h = Hash("type", "subscribeNetwork")
        h["channelName"] = (box.configuration.id + ":" +
                            ".".join(box.path))
        h["subscribe"] = subscribe
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
        loginInfo.set("host", socket.gethostname())
        loginInfo.set("pid", QCoreApplication.applicationPid())
        loginInfo.set("version", globals.GUI_VERSION)
        self._tcpWriteHash(loginInfo)


    def _handleBrokerInformation(self, host, port, topic):
        self.brokerHost = host
        self.brokerPort = port
        self.brokerTopic = topic
        self._login()


network = _Network()

def Network():
    return network
