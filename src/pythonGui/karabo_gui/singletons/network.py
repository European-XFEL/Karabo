#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from functools import partial
import os.path as op
import socket
from struct import calcsize, pack, unpack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, QByteArray, QCoreApplication,
                          QCryptographicHash, QObject)
from PyQt4.QtGui import QDialog, QMessageBox

from karabo.authenticator import Authenticator
from karabo.common.api import ShellNamespaceWrapper
from karabo.middlelayer import Hash, BinaryWriter, AccessLevel
from karabo_gui import background
from karabo_gui.dialogs.logindialog import LoginDialog
import karabo_gui.globals as krb_globals


class Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()
    signalReceivedData = pyqtSignal(object)

    def __init__(self, parent=None):
        super(Network, self).__init__(parent=parent)

        self.authenticator = None

        self.brokerHost = ""
        self.brokerPort = ""
        self.brokerTopic = ""
        self.sessionToken = ""

        self.tcpSocket = None
        self.requestQueue = []

        self.username = None
        self.password = None
        self.provider = None
        self.hostname, self.port = self.load_settings()

    def connectToServer(self):
        """
        Connection to server via LoginDialog.
        """
        isConnected = False

        dialog = LoginDialog(username=self.username,
                             password=self.password,
                             provider=self.provider)

        if dialog.exec_() == QDialog.Accepted:
            self.username = dialog.username
            self.password = dialog.password
            self.provider = dialog.provider
            self.startServerConnection()
            isConnected = True

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(isConnected)

    def load_settings(self):
        hostname = "localhost"
        port = 44444

        if op.exists(krb_globals.CONFIG_FILE):
            config = ShellNamespaceWrapper(krb_globals.CONFIG_FILE)
            hostname = config.get(krb_globals.KARABO_GUI_HOST, hostname)
            port = int(config.get(krb_globals.KARABO_GUI_PORT, port))

        return (hostname, port)

    def disconnectFromServer(self):
        """
        Disconnect from server.
        """
        # All panels need to be reseted and all projects closed
        self.signalServerConnectionChanged.emit(False)
        self.endServerConnection()

    def startServerConnection(self):
        """
        Attempt to connect to host on given port and save user specific data
        for later authentification.
        """

        self.tcpSocket = QTcpSocket(self)
        self.tcpSocket.connected.connect(self.onConnected)
        self.tcpSocket.readyRead.connect(self.onReadServerData)
        self.tcpSocket.disconnected.connect(self.onDisconnected)
        self.tcpSocket.error.connect(self.onSocketError)
        self.tcpSocket.connectToHost(self.hostname, self.port)

    def endServerConnection(self):
        """
        End connection to server and database.
        """
        self._logout()
        self.requestQueue = []

        if self.tcpSocket is None:
            return

        self.tcpSocket.disconnectFromHost()
        if (self.tcpSocket.state() == QAbstractSocket.UnconnectedState or
                self.tcpSocket.waitForDisconnected(5000)):
            return

        print("Disconnect failed:", self.tcpSocket.errorString())

    def _login(self):
        """
        Authentification login.
        """
        # System variables definition
        ipAddress = socket.gethostname()  # Machine Name

        # Easteregg
        if self.username == "god":
            md5 = QCryptographicHash.hash(str(self.password), QCryptographicHash.Md5).toHex()
            if md5 == "39d676ecced45b02da1fb45731790b4c":
                print("Entering god mode...")
                krb_globals.GLOBAL_ACCESS_LEVEL = AccessLevel.GOD
            else:
                krb_globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER

        else:
            # Construct Authenticator class
            try:
                # TODO: adapt Authenticator constructor for unicode parameters
                # instead of string
                self.authenticator = Authenticator(
                    self.username, self.password, self.provider, ipAddress,
                    self.brokerHost, self.brokerPort, self.brokerTopic)
            except Exception as e:
                QMessageBox.warning(None, 'Authenticator not available',
                                    str(e))
                return

            # Execute Login
            ok = False
            try:
                ok = self.authenticator.login()
            except Exception as e:
                # TODO Fall back to inbuild access level
                krb_globals.GLOBAL_ACCESS_LEVEL = krb_globals.KARABO_DEFAULT_ACCESS_LEVEL
                print("Login problem. Please verify, if service is running. " + str(e))

                # Inform the mainwindow to change correspondingly the allowed level-downgrade
                self.signalUserChanged.emit()
                self._sendLoginInformation(self.username, self.password,
                                           self.provider, self.sessionToken)
                return

            if ok:
                krb_globals.GLOBAL_ACCESS_LEVEL = AccessLevel(
                    self.authenticator.defaultAccessLevelId)
            else:
                print("Login failed")
                self.onSocketError(QAbstractSocket.ConnectionRefusedError)
                # krb_globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
                return

        # Inform the mainwindow to change correspondingly the allowed level-downgrade
        self.signalUserChanged.emit()
        self._sendLoginInformation(self.username, self.password, self.provider,
                                   self.sessionToken)

    def _logout(self):
        """
        Authentification logout.
        """
        # Execute Logout
        if self.authenticator is None:
            return

        try:
            self.authenticator.logout()
        except Exception as e:
            print("Logout problem. Please verify, if service is running. " + str(e))

    def onReadServerData(self):
        """ Run the network reader generator until it yields.
        """
        # self.dataReader is a generator object from self._networkReadGenerator
        # Stepping it with next() causes network data to be read.
        next(self.dataReader)

    def _networkReadGenerator(self):
        """ A generator which continuously reads GUI server messages and yields
        when there isn't enough buffered data to be read.
        """
        sizeFormat = 'I'  # unsigned int
        bytesNeededSize = calcsize(sizeFormat)

        while True:
            # Read the size of the Hash
            while self.tcpSocket.bytesAvailable() < bytesNeededSize:
                yield
            rawBytesNeeded = self.tcpSocket.read(bytesNeededSize)
            bytesNeeded = unpack(sizeFormat, rawBytesNeeded)[0]

            # Read the Hash
            while self.tcpSocket.bytesAvailable() < bytesNeeded:
                yield
            dataBytes = self.tcpSocket.read(bytesNeeded)

            # Do something with the Hash at some point in the future.
            background.executeLater(partial(self.parseInput, dataBytes),
                                    background.Priority.NETWORK)

    def parseInput(self, data):
        """parse the data and emit the signalReceivedData"""
        self.signalReceivedData.emit(Hash.decode(data, "Bin"))

    def onSocketError(self, socketError):
        print("onSocketError", self.tcpSocket.errorString(), socketError)

        self.disconnectFromServer()

        if socketError == QAbstractSocket.ConnectionRefusedError:
            msg = ('The connection to GUI server <b>{}[:{}]</b> '
                   'failed.').format(self.hostname, self.port)
            reply = QMessageBox.question(
                None, 'Server connection refused', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.RemoteHostClosedError:
            msg = ('The remote host <b>{}[:{}]</b> closed the connection'
                   '.').format(self.hostname, self.port)
            reply = QMessageBox.question(
                None, 'Connection closed', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.HostNotFoundError:
            msg = 'The host address <b>{}</b> was not found.'.format(
                self.hostname)
            reply = QMessageBox.question(
                None, 'Host address error', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        elif socketError == QAbstractSocket.NetworkError:
            msg = ('An error occurred with the network (e.g., <br>the network '
                   'cable was accidentally plugged out).')
            reply = QMessageBox.question(
                None, 'Network error', msg,
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
        self.dataReader = self._networkReadGenerator()

    def onDisconnected(self):
        pass

    def onKillDevice(self, deviceId):
        h = Hash("type", "killDevice")
        h.set("deviceId", deviceId)
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

    def onInitDevice(self, serverId, classId, deviceId, config, attrUpdates=None):
        h = Hash("type", "initDevice")
        h.set("serverId", serverId)
        h.set("classId", classId)
        h.set("deviceId", deviceId)
        h.set("configuration", config)
        if attrUpdates is not None:
            h.set("schemaUpdates", attrUpdates)
        self._tcpWriteHash(h)

    def onExecute(self, box, *args):
        h = Hash("type", "execute")
        h.set("deviceId", box.configuration.id)
        h.set("command", ".".join(box.path))

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

    # ---------------------------------------------------------------------
    # Current Project Interface

    def onProjectBeginSession(self, project_manager):
        h = Hash("type", "projectBeginUserSession")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        self._tcpWriteHash(h)

    def onProjectEndSession(self, project_manager):
        h = Hash("type", "projectEndUserSession")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        self._tcpWriteHash(h)

    def onListProjectDomains(self, project_manager):
        h = Hash("type", "projectListDomains")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        self._tcpWriteHash(h)

    def onListProjectManagers(self):
        h = Hash("type", "projectListProjectManagers")
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        self._tcpWriteHash(h)

    def onProjectListItems(self, project_manager, domain, item_type):
        h = Hash("type", "projectListItems")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        h.set("domain", domain)
        h.set("item_types", [item_type])
        self._tcpWriteHash(h)

    def onProjectLoadItems(self, project_manager, items):
        h = Hash("type", "projectLoadItems")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        h.set("items", items)
        self._tcpWriteHash(h)

    def onProjectSaveItems(self, project_manager, items):
        h = Hash("type", "projectSaveItems")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        h.set("items", items)
        self._tcpWriteHash(h)

    # ---------------------------------------------------------------------
    # Legacy Project Interface

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

    # ---------------------------------------------------------------------

    def onAcknowledgeAlarm(self, instanceId, rowId):
        h = Hash("type", "acknowledgeAlarm")
        h.set("alarmInstanceId", instanceId)
        h.set("acknowledgedRows", Hash(rowId, True))
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

    # ---------------------------------------------------------------------
    # Runconfiguration
    def onSourcesInGroup(self, instanceId, group):
        h = Hash("type", "runConfigSourcesInGroup")
        h.set("runConfiguratorId", instanceId)
        h.set("group", group)
        self._tcpWriteHash(h)

# --------------------------------------------------------------------------
# private functions

    def _tcpWriteHash(self, h):
        # There might be a connect to server in progress, but without success
        if (self.tcpSocket is None or
                self.tcpSocket.state() == QAbstractSocket.HostLookupState or
                self.tcpSocket.state() == QAbstractSocket.ConnectingState):
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
        loginInfo.set("version", krb_globals.GUI_VERSION)
        self._tcpWriteHash(loginInfo)

    def _handleBrokerInformation(self, host, port, topic):
        self.brokerHost = host
        self.brokerPort = port
        self.brokerTopic = topic
        self._login()
