#############################################################################
# Author: <burkhard.heisen@xfel.eu>
# Created on February 17, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from functools import partial
import os
import socket
from struct import calcsize, pack, unpack

from PyQt4.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt4.QtCore import (pyqtSignal, QByteArray, QCoreApplication,
                          QCryptographicHash, QObject)
from PyQt4.QtGui import QDialog, QMessageBox, qApp

from karabo.authenticator import Authenticator
from karabo.middlelayer import (
    Hash, decodeBinary, encodeBinary, AccessLevel, Timestamp)
from karabo_gui import background
from karabo_gui.dialogs.logindialog import LoginDialog
from karabo_gui.enums import KaraboSettings
from karabo_gui.events import broadcast_event, KaraboEventSender
import karabo_gui.globals as krb_globals
from karabo_gui.util import get_setting, set_setting


class Network(QObject):
    # signals
    signalServerConnectionChanged = pyqtSignal(bool)
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

        self._waitingMessages = {}
        self._monitoringPerformance = False

        self.username = ''
        self.password = ''
        self.provider = ''
        self.hostname, self.port = self.load_settings()

        # Listen for the quit notification
        qApp.aboutToQuit.connect(self.onQuitApplication)

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
            self.username = dialog.username
            self.password = dialog.password
            self.provider = dialog.provider
            self.hostname = dialog.hostname
            self.port = dialog.port
            self.startServerConnection()
            isConnected = True

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(isConnected)

    def load_settings(self):
        # Try to load from the environment variables first
        hostname = os.environ.get(krb_globals.KARABO_GUI_HOST, None)
        port = os.environ.get(krb_globals.KARABO_GUI_PORT, None)
        if hostname is not None and port is not None:
            return (hostname, port)

        # Load from QSettings
        server = get_setting(KaraboSettings.GUI_SERVER)
        server = server or 'localhost:44444'
        hostname, port = server.split(':')
        port = int(port)

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

    def togglePerformanceMonitor(self):
        self._monitoringPerformance = not self._monitoringPerformance

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
            # Execute Login
            ok = False
            try:
                self.authenticator = Authenticator(
                    self.username, self.password, self.provider, ipAddress,
                    self.brokerHost, self.brokerPort, self.brokerTopic)
                ok = self.authenticator.login()
            except Exception as e:
                # TODO Fall back to inbuild access level
                krb_globals.GLOBAL_ACCESS_LEVEL = krb_globals.KARABO_DEFAULT_ACCESS_LEVEL
                print("Login problem. Please verify, if service is running. " + str(e))

                # Inform the GUI to change correspondingly the allowed level-downgrade
                broadcast_event(KaraboEventSender.LoginUserChanged, {})
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

        # Inform the GUI to change correspondingly the allowed level-downgrade
        broadcast_event(KaraboEventSender.LoginUserChanged, {})
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
            self._waitingMessages[id(dataBytes)] = Timestamp().toTimestamp()
            background.executeLater(partial(self.parseInput, dataBytes),
                                    background.Priority.NETWORK)

    def parseInput(self, data):
        """parse the data and emit the signalReceivedData"""
        self._performanceMonitor(self._waitingMessages.pop(id(data)))
        self.signalReceivedData.emit(decodeBinary(data))

    def _performanceMonitor(self, recv_timestamp):
        if self._monitoringPerformance:
            diff = Timestamp().toTimestamp() - recv_timestamp
            broadcast_event(KaraboEventSender.ProcessingDelay, {'value': diff})

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
        # cache the server address
        server = '{}:{}'.format(self.hostname, self.port)
        set_setting(KaraboSettings.GUI_SERVER, server)

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
            try:
                value = box.descriptor.cast(value)
            except ValueError:
                continue
            conf[".".join(box.path)] = value

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

    def onExecute(self, device_id, slot_name):
        h = Hash("type", "execute")
        h.set("deviceId", device_id)
        h.set("command", slot_name)

        self._tcpWriteHash(h)

    def onExecuteGeneric(self, token, device_id, slot_name, params):
        h = Hash("type", "requestFromSlot")
        h.set("deviceId", device_id)
        h.set("slot", slot_name)
        h.set("args", params)
        h.set("token", token)

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

    def onProjectUpdateAttribute(self, project_manager, items):
        h = Hash("type", "projectUpdateAttribute")
        h.set("projectManager", project_manager)
        # XXX: Don't leave token hardcoded!
        h.set("token", "admin")
        h.set("items", items)
        self._tcpWriteHash(h)

    # ---------------------------------------------------------------------

    def onAcknowledgeAlarm(self, instanceId, rowId):
        h = Hash("type", "acknowledgeAlarm")
        h.set("alarmInstanceId", instanceId)
        h.set("acknowledgedRows", Hash(rowId, True))
        self._tcpWriteHash(h)

    def onRequestAlarms(self, instanceId):
        h = Hash("type", "requestAlarms")
        h.set("alarmInstanceId", instanceId)
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
        dataBytes = encodeBinary(h)
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
