from functools import partial
import socket
from struct import calcsize, pack, unpack

from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket
from PyQt5.QtCore import (
    pyqtSignal, pyqtSlot, QByteArray, QCoreApplication, QObject)
from PyQt5.QtWidgets import QDialog, QMessageBox, qApp

from karabo.native import (
    AccessLevel, decodeBinary, encodeBinary, Hash, Timestamp)
from karabogui import background
from karabogui.const import REQUEST_REPLY_TIMEOUT
from karabogui.dialogs.logindialog import LoginDialog
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.singletons.api import get_config
import karabogui.globals as krb_globals

ACCESS_LEVEL_MAP = {
    "observer": 0,
    "user": 1,
    "operator": 2,
    "expert": 3,
    "admin": 4,
    "god": 5}


class Network(QObject):
    # our qt signals
    signalServerConnectionChanged = pyqtSignal(bool)
    signalReceivedData = pyqtSignal(object)
    signalNetworkPerformance = pyqtSignal(float, bool)

    def __init__(self, parent=None):
        super(Network, self).__init__(parent=parent)
        self.sessionToken = ""

        self.tcpSocket = None
        self.requestQueue = []

        self._waitingMessages = {}
        self._show_proc_delay = False

        self.hostname = "localhost"
        self.port = "44444"
        self.password = "karabo"
        self.provider = "LOCAL"
        self.max_servers = 5
        self.load_login_settings()

        # Listen for the quit notification
        qApp.aboutToQuit.connect(self.onQuitApplication)

    def connectToServer(self, parent=None):
        """Connection to server via LoginDialog
        """
        isConnected = False

        dialog = LoginDialog(username=self.username,
                             password=self.password,
                             provider=self.provider,
                             hostname=self.hostname,
                             port=self.port,
                             guiservers=self.guiservers,
                             parent=parent)

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
        # Allow external runner to see the status of the connection!
        return isConnected

    def connectToServerDirectly(self, username, hostname, port):
        """Connection to server directly via username, host and port

        This function is used for fast startup via the karabo cinema
        """
        self.username = username
        self.hostname = hostname
        self.port = port
        self.startServerConnection()

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(True)
        # Allow external runner to see the status of the connection!
        return True

    def load_login_settings(self):
        # Maximum number of GUI servers to be stored, 5 by default

        # Load from configuration singleton
        config = get_config()
        self.username = config['username']
        self.guiservers = config['gui_servers']

        if self.guiservers:
            self.hostname, self.port = self.guiservers[0].split(':')

        self.port = int(self.port)

    def disconnectFromServer(self):
        """Disconnect from server
        """
        # All panels need to be reseted and all projects closed
        self.signalServerConnectionChanged.emit(False)
        self.endServerConnection()

    def startServerConnection(self):
        """Create the tcp socket and connect to hostname and port
        """
        self.tcpSocket = QTcpSocket(self)
        self.tcpSocket.connected.connect(self.onConnected)
        self.tcpSocket.readyRead.connect(self.onReadServerData)
        self.tcpSocket.disconnected.connect(self.onDisconnected)
        self.tcpSocket.error.connect(self.onSocketError)
        self.tcpSocket.connectToHost(self.hostname, self.port)

    def endServerConnection(self):
        """End connection to server and database
        """
        self.requestQueue = []

        if self.tcpSocket is None:
            return

        self.tcpSocket.disconnectFromHost()
        if (self.tcpSocket.state() == QAbstractSocket.UnconnectedState or
                self.tcpSocket.waitForDisconnected(5000)):
            return

        print("Disconnect failed:", self.tcpSocket.errorString())

    def togglePerformanceMonitor(self):
        self._show_proc_delay = not self._show_proc_delay

    @pyqtSlot()
    def onReadServerData(self):
        """Run the network reader generator until it yields.
        """
        # self.dataReader is a generator object from self._networkReadGenerator
        # Stepping it with next() causes network data to be read.
        next(self.dataReader)

    def _networkReadGenerator(self):
        """A generator which continuously reads GUI server messages

        The generator yields when there isn't enough buffered data to be read.
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
        diff = Timestamp().toTimestamp() - recv_timestamp
        self.signalNetworkPerformance.emit(diff, self._show_proc_delay)

    @pyqtSlot(QAbstractSocket.SocketError)
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
            msg = ('The remote host <b>{}[:{}]</b> closed the '
                   'connection').format(self.hostname, self.port)
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
        elif socketError == QAbstractSocket.SocketAccessError:
            msg = ('The socket operation failed because the application '
                   'lacked the required privileges.')
            reply = QMessageBox.question(
                None, 'Network error', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)

            if reply == QMessageBox.Cancel:
                return
        else:
            msg = ('An unknown socket operation error occured. '
                   'Type: {} - Description: {}').format(
                socketError, self.tcpSocket.errorString())
            reply = QMessageBox.question(
                None, 'Network error', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)
            if reply == QMessageBox.Cancel:
                return

        self.connectToServer()

    def onServerConnection(self, connect, parent=None):
        """Connect or disconnect depending on the input parameter

        :param connect: Either True or False
        :type connect: bool
        """
        if connect:
            self.connectToServer(parent)
        else:
            self.disconnectFromServer()

    @pyqtSlot()
    def onQuitApplication(self):
        """This slot is triggered from the MainWindow

        The user wants to quit the application so the network connection
        needs to be closed and the manager needs to be informed to close
        the database connection as well.
        """
        self.endServerConnection()

    @pyqtSlot()
    def onConnected(self):
        def _least_recently_used(item, sequence, maxsize):
            """Apply of the Least Recently Used (LRU) algorithm on sequence

            The LRU algorithm evicts from sequence the least recently used
            element and item is moved to the sequence front.
            As the result, sequence always contains its most popular element
            """
            if item in sequence:
                sequence.remove(item)
            sequence = [item] + sequence
            return sequence if len(sequence) <= maxsize else sequence[:maxsize]

        # cache the server address
        server = '{}:{}'.format(self.hostname, self.port)

        self.guiservers = _least_recently_used(server, self.guiservers,
                                               int(self.max_servers))

        # Save to singleton!
        get_config()['username'] = self.username
        get_config()['gui_servers'] = self.guiservers

        # If some requests got piled up, because of no server connection,
        # now these get handled
        for r in self.requestQueue:
            self._tcpWriteHash(r)
        self.requestQueue = []
        self.dataReader = self._networkReadGenerator()

    @pyqtSlot()
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
        h["reply"] = True
        h["timeout"] = REQUEST_REPLY_TIMEOUT
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

    def onExecute(self, device_id, slot_name, ignore_timeouts):
        h = Hash("type", "execute")
        h["deviceId"] = device_id
        h["command"] = slot_name
        h["reply"] = True
        if not ignore_timeouts:
            h["timeout"] = REQUEST_REPLY_TIMEOUT
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

    def onGetConfigurationFromPast(self, device_id, time):
        h = Hash("type", "getConfigurationFromPast")
        h["deviceId"] = device_id
        h["time"] = time
        self._tcpWriteHash(h)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def onProjectBeginSession(self, project_manager):
        h = Hash("type", "projectBeginUserSession")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._tcpWriteHash(h)

    def onProjectEndSession(self, project_manager):
        h = Hash("type", "projectEndUserSession")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._tcpWriteHash(h)

    def onListProjectDomains(self, project_manager):
        h = Hash("type", "projectListDomains")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._tcpWriteHash(h)

    def onListProjectManagers(self):
        h = Hash("type", "projectListProjectManagers")
        h["token"] = get_config()["db_token"]
        self._tcpWriteHash(h)

    def onProjectListItems(self, project_manager, domain, item_type):
        h = Hash("type", "projectListItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["domain"] = domain
        h["item_types"] = [item_type]
        self._tcpWriteHash(h)

    def onProjectLoadItems(self, project_manager, items):
        h = Hash("type", "projectLoadItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["items"] = items
        self._tcpWriteHash(h)

    def onProjectSaveItems(self, project_manager, items):
        h = Hash("type", "projectSaveItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["items"] = items
        self._tcpWriteHash(h)

    def onProjectUpdateAttribute(self, project_manager, items):
        h = Hash("type", "projectUpdateAttribute")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
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

    def onRequestNetwork(self, name):
        h = Hash("type", "requestNetwork", "channelName", name)
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

    def set_server_information(self, read_only=False, **kwargs):
        """We get the reply from the GUI Server and set the information"""
        if read_only:
            default = AccessLevel.OBSERVER
            self.username = "observer"
        else:
            default = AccessLevel(ACCESS_LEVEL_MAP.get(
                self.username, AccessLevel.ADMIN))

        krb_globals.GLOBAL_ACCESS_LEVEL = default
        # Inform the GUI to change correspondingly the allowed
        # level-downgrade
        broadcast_event(KaraboEvent.LoginUserChanged, {})
        self._sendLoginInformation(self.username, self.password,
                                   self.provider, self.sessionToken)
