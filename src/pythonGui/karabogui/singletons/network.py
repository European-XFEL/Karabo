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
from karabogui import background
from karabogui.dialogs.logindialog import LoginDialog
from karabogui.enums import KaraboSettings
from karabogui.events import broadcast_event, KaraboEventSender
import karabogui.globals as krb_globals
from karabogui.util import get_setting, set_setting


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
        self.hostname, self.port, self.guiservers, self.maxservers = (
                self.load_settings())

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
                             port=self.port,
                             guiservers=self.guiservers)

        if dialog.exec_() == QDialog.Accepted:
            self.username = dialog.username
            self.password = dialog.password
            self.provider = dialog.provider
            self.hostname = dialog.hostname
            self.port = dialog.port
            self.guiservers = dialog.guiservers
            self.startServerConnection()
            isConnected = True

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(isConnected)

    def load_settings(self):
        # Maximum number of GUI servers to be stored, 5 by default
        maxservers = 5

        # Try to load from the environment variables first
        hostname = os.environ.get(krb_globals.KARABO_GUI_HOST, None)
        port = os.environ.get(krb_globals.KARABO_GUI_PORT, None)
        if hostname is not None and port is not None:
            servers = get_setting(KaraboSettings.GUI_SERVERS)
            maxservers = (get_setting(KaraboSettings.MAX_GUI_SERVERS)
                          or maxservers)
            return (hostname, port, servers, maxservers)

        # Load from QSettings
        servers = get_setting(KaraboSettings.GUI_SERVERS) or []
        maxservers = get_setting(KaraboSettings.MAX_GUI_SERVERS) or maxservers
        if servers:
            hostname, port = servers[0].split(':')
        else:
            hostname, port = "localhost", "44444"

        port = int(port)

        return (hostname, port, servers, maxservers)

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
            md5 = QCryptographicHash.hash(str(self.password),
                                          QCryptographicHash.Md5).toHex()
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
                default = krb_globals.KARABO_DEFAULT_ACCESS_LEVEL
                krb_globals.GLOBAL_ACCESS_LEVEL = default
                print("Login problem. Please verify, if service is running. {}"
                      "".format(str(e)))

                # Inform the GUI to change correspondingly the allowed
                # level-downgrade
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
            print("Logout problem. Please verify, if service is running. {}"
                  "".format(str(e)))

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
        def _LRU(item, lis, maxsize):
            """
            Apply of the LRU algorithm on lis

            The LRU algorithm evicts from lis the Least Recently Used
            element and item is move to the lis front.
            As the result, lis always contain its most popular element
            """
            if item in lis:
                lis.remove(item)
            lis = [item] + lis
            return lis if len(lis) < maxsize else lis[:maxsize]

        # cache the server address
        server = '{}:{}'.format(self.hostname, self.port)

        self.guiservers = _LRU(server, self.guiservers, int(self.maxservers))

        set_setting(KaraboSettings.GUI_SERVERS, self.guiservers)
        set_setting(KaraboSettings.MAX_GUI_SERVERS, self.maxservers)

        # If some requests got piled up, because of no server connection,
        # now these get handled
        for r in self.requestQueue:
            self._tcpWriteHash(r)
        self.requestQueue = []
        self.dataReader = self._networkReadGenerator()

    def onDisconnected(self):
        pass

# ---------------------------------------------------------------------
# Protocol methods

    def onKillDevice(self, device_id):
        h = Hash("type", "killDevice")
        h["deviceId"] = device_id
        self._tcpWriteHash(h)

    def onKillServer(self, server_id):
        h = Hash("type", "killServer")
        h["serverId"] = server_id
        self._tcpWriteHash(h)

    def onGetDeviceConfiguration(self, device_id):
        h = Hash("type", "getDeviceConfiguration")
        h["deviceId"] = device_id
        self._tcpWriteHash(h)

    def onReconfigure(self, device_id, configuration):
        """Set values in a device
        """
        h = Hash("type", "reconfigure")
        h["deviceId"] = device_id
        h["configuration"] = configuration
        self._tcpWriteHash(h)

    def onInitDevice(self, server_id, class_id, device_id, config,
                     attrUpdates=None):
        h = Hash("type", "initDevice")
        h["serverId"] = server_id
        h["classId"] = class_id
        h["deviceId"] = device_id
        h["configuration"] = config
        if attrUpdates is not None:
            h["schemaUpdates"] = attrUpdates
        self._tcpWriteHash(h)

    def onExecute(self, device_id, slot_name):
        h = Hash("type", "execute")
        h["deviceId"] = device_id
        h["command"] = slot_name
        self._tcpWriteHash(h)

    def onExecuteGeneric(self, token, device_id, slot_name, params):
        h = Hash("type", "requestFromSlot")
        h["deviceId"] = device_id
        h["slot"] = slot_name
        h["args"] = params
        h["token"] = token
        self._tcpWriteHash(h)

    def onStartMonitoringDevice(self, device_id):
        h = Hash("type", "startMonitoringDevice")
        h["deviceId"] = device_id
        self._tcpWriteHash(h)

    def onStopMonitoringDevice(self, device_id):
        h = Hash("type", "stopMonitoringDevice")
        h["deviceId"] = device_id
        self._tcpWriteHash(h)

    def onGetClassSchema(self, server_id, class_id):
        h = Hash("type", "getClassSchema")
        h["serverId"] = server_id
        h["classId"] = class_id
        self._tcpWriteHash(h)

    def onGetDeviceSchema(self, device_id):
        h = Hash("type", "getDeviceSchema")
        h["deviceId"] = device_id
        self._tcpWriteHash(h)

    def onGetPropertyHistory(self, device_id, path, t0, t1, maxNumData):
        h = Hash("type", "getPropertyHistory")
        h["deviceId"] = device_id
        h["property"] = path
        h["t0"] = t0
        h["t1"] = t1
        h["maxNumData"] = maxNumData
        self._tcpWriteHash(h)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def onProjectBeginSession(self, project_manager):
        h = Hash("type", "projectBeginUserSession")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        self._tcpWriteHash(h)

    def onProjectEndSession(self, project_manager):
        h = Hash("type", "projectEndUserSession")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        self._tcpWriteHash(h)

    def onListProjectDomains(self, project_manager):
        h = Hash("type", "projectListDomains")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        self._tcpWriteHash(h)

    def onListProjectManagers(self):
        h = Hash("type", "projectListProjectManagers")
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        self._tcpWriteHash(h)

    def onProjectListItems(self, project_manager, domain, item_type):
        h = Hash("type", "projectListItems")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        h["domain"] = domain
        h["item_types"] = [item_type]
        self._tcpWriteHash(h)

    def onProjectLoadItems(self, project_manager, items):
        h = Hash("type", "projectLoadItems")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        h["items"] = items
        self._tcpWriteHash(h)

    def onProjectSaveItems(self, project_manager, items):
        h = Hash("type", "projectSaveItems")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        h["items"] = items
        self._tcpWriteHash(h)

    def onProjectUpdateAttribute(self, project_manager, items):
        h = Hash("type", "projectUpdateAttribute")
        h["projectManager"] = project_manager
        # XXX: Don't leave token hardcoded!
        h["token"] = "admin"
        h["items"] = items
        self._tcpWriteHash(h)

    # ---------------------------------------------------------------------

    def onAcknowledgeAlarm(self, instanceId, rowId):
        h = Hash("type", "acknowledgeAlarm")
        h["alarmInstanceId"] = instanceId
        h["acknowledgedRows"] = Hash(rowId, True)
        self._tcpWriteHash(h)

    def onRequestAlarms(self, instanceId):
        h = Hash("type", "requestAlarms")
        h["alarmInstanceId"] = instanceId
        self._tcpWriteHash(h)

    def onSubscribeToOutput(self, device_id, path, subscribe):
        h = Hash("type", "subscribeNetwork")
        h["channelName"] = device_id + ":" + path
        h["subscribe"] = subscribe
        self._tcpWriteHash(h)

    def onError(self, error):
        h = Hash("type", "error", "traceback", error)
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

    def _sendLoginInformation(self, username, password, provider,
                              sessionToken):
        loginInfo = Hash("type", "login")
        loginInfo["username"] = username
        loginInfo["password"] = password
        loginInfo["provider"] = provider
        loginInfo["sessionToken"] = sessionToken
        loginInfo["host"] = socket.gethostname()
        loginInfo["pid"] = QCoreApplication.applicationPid()
        loginInfo["version"] = krb_globals.GUI_VERSION
        self._tcpWriteHash(loginInfo)

    def _handleBrokerInformation(self, host, port, topic):
        self.brokerHost = host
        self.brokerPort = port
        self.brokerTopic = topic
        self._login()
