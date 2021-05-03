from functools import partial
from struct import calcsize, pack, unpack

from qtpy.QtNetwork import QAbstractSocket, QTcpSocket
from qtpy.QtCore import Signal, Slot, QByteArray,  QObject
from qtpy.QtWidgets import QDialog, QMessageBox, qApp

from karabo.common.api import KARABO_CONFIG_MANAGER
from karabo.native import (
    AccessLevel, decodeBinary, encodeBinary, Hash, Timestamp)
from karabogui import background
from karabogui.const import REQUEST_REPLY_TIMEOUT
from karabogui.dialogs.logindialog import LoginDialog
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.logger import get_logger
from karabogui.singletons.api import get_config
from karabogui.util import process_qt_events
import karabogui.globals as krb_globals

ACCESS_LEVEL_MAP = {
    "observer": 0,
    "user": 1,
    "operator": 2,
    "expert": 3,
    "admin": 4,
    "god": 5}

MAX_GUI_SERVER_HISTORY = 5
logger = get_logger()


class Network(QObject):
    """The `Network` class is the singleton holding the tcp socket for
    the gui server connection.
    """
    signalServerConnectionChanged = Signal(bool)
    signalReceivedData = Signal(object)
    signalNetworkPerformance = Signal(float, bool)

    def __init__(self, parent=None):
        super(Network, self).__init__(parent=parent)

        self._tcp_socket = None
        self._data_reader = None
        self._request_queue = []

        self._waiting_messages = {}
        self._show_proc_delay = False

        self.username = "operator"
        self.gui_servers = []
        self.hostname = "localhost"
        self.port = "44444"
        self.password = "karabo"

        # Check default settings stored in QSettings!
        self._load_login_settings()

        # Listen for the quit notification
        qApp.aboutToQuit.connect(self.onQuitApplication)

    def connectToServer(self, parent=None):
        """Connection to server via LoginDialog"""
        connected = False

        dialog = LoginDialog(username=self.username,
                             password=self.password,
                             hostname=self.hostname,
                             port=self.port,
                             gui_servers=self.gui_servers,
                             parent=parent)

        if dialog.exec_() == QDialog.Accepted:
            self.username = dialog.username
            self.password = dialog.password
            self.hostname = dialog.hostname
            self.port = dialog.port
            self.gui_servers = dialog.gui_servers
            self.startServerConnection()
            connected = True

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(connected)
        # Allow external runner to see the status of the connection!
        return connected

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

    def disconnectFromServer(self):
        """Disconnect from server"""
        # All panels need to be reset and all projects closed
        self.signalServerConnectionChanged.emit(False)
        process_qt_events(timeout=5000)
        self.endServerConnection()

    def startServerConnection(self):
        """Create the tcp socket and connect to hostname and port"""
        self._tcp_socket = QTcpSocket(self)
        self._tcp_socket.connected.connect(self.onConnected)
        self._tcp_socket.readyRead.connect(self.onReadServerData)
        self._tcp_socket.disconnected.connect(self.onDisconnected)
        self._tcp_socket.error.connect(self.onSocketError)
        self._tcp_socket.connectToHost(self.hostname, self.port)

    def endServerConnection(self):
        """End connection to server and database"""
        self._request_queue = []

        if self._tcp_socket is None:
            return

        self._tcp_socket.disconnectFromHost()
        if (self._tcp_socket.state() == QAbstractSocket.UnconnectedState or
                self._tcp_socket.waitForDisconnected(5000)):
            return

        print("Disconnect failed:", self._tcp_socket.errorString())

    def togglePerformanceMonitor(self):
        """External method to toggle the performance monitor"""
        self._show_proc_delay = not self._show_proc_delay

    @Slot()
    def onReadServerData(self):
        """Run the network reader generator until it yields"""
        # self._data_reader is a generator object from self._network_generator.
        # Stepping it with next() causes network data to be read.
        next(self._data_reader)

    def _network_generator(self):
        """A generator which continuously reads GUI server messages

        The generator yields when there isn't enough buffered data to be read.
        """
        sizeFormat = 'I'  # unsigned int
        bytesNeededSize = calcsize(sizeFormat)

        while True:
            # Read the size of the Hash
            while self._tcp_socket.bytesAvailable() < bytesNeededSize:
                yield
            rawBytesNeeded = self._tcp_socket.read(bytesNeededSize)
            bytesNeeded = unpack(sizeFormat, rawBytesNeeded)[0]

            # Read the Hash
            while self._tcp_socket.bytesAvailable() < bytesNeeded:
                yield
            dataBytes = self._tcp_socket.read(bytesNeeded)

            # Do something with the Hash at some point in the future.
            self._waiting_messages[id(dataBytes)] = Timestamp().toTimestamp()
            background.executeLater(partial(self.parseInput, dataBytes),
                                    background.Priority.NETWORK)

    def parseInput(self, data):
        """parse the data and emit the signalReceivedData"""
        self._performance_monitor(self._waiting_messages.pop(id(data)))
        self.signalReceivedData.emit(decodeBinary(data))

    @Slot(QAbstractSocket.SocketError)
    def onSocketError(self, socketError):
        print("onSocketError", self._tcp_socket.errorString(), socketError)

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
                socketError, self._tcp_socket.errorString())
            reply = QMessageBox.question(
                None, 'Network error', msg,
                QMessageBox.Retry | QMessageBox.Cancel, QMessageBox.Retry)
            if reply == QMessageBox.Cancel:
                return

        self.connectToServer()

    def onServerConnection(self, connect, parent=None):
        """Connect or disconnect depending on the input parameter

        This function is called from the main window directly!

        :param connect: Either True or False
        :type connect: bool
        """
        if connect:
            self.connectToServer(parent)
        else:
            self.disconnectFromServer()

    @Slot()
    def onQuitApplication(self):
        """This slot is triggered from the MainWindow

        The user wants to quit the application so the network connection
        needs to be closed and the manager needs to be informed to close
        the database connection as well.
        """
        self.endServerConnection()

    @Slot()
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

        self.gui_servers = _least_recently_used(server, self.gui_servers,
                                                int(MAX_GUI_SERVER_HISTORY))

        # Save to singleton!
        get_config()['username'] = self.username
        get_config()['gui_servers'] = self.gui_servers

        # If some requests got piled up, because of no server connection,
        # now these get handled
        for r in self._request_queue:
            self._write_hash(r)
        self._request_queue = []
        self._data_reader = self._network_generator()

    @Slot()
    def onDisconnected(self):
        """The tcp socket was disconnected"""

    # ---------------------------------------------------------------------
    # Protocol methods

    def onKillDevice(self, device_id):
        logger.info(f"Sending request to shutdown device <b>{device_id}</b>")
        h = Hash("type", "killDevice")
        h["deviceId"] = device_id
        self._write_hash(h)

    def onKillServer(self, server_id):
        logger.info(f"Sending request to shutdown server <b>{server_id}</b>")
        h = Hash("type", "killServer")
        h["serverId"] = server_id
        self._write_hash(h)

    def onGetDeviceConfiguration(self, device_id):
        h = Hash("type", "getDeviceConfiguration")
        h["deviceId"] = device_id
        self._write_hash(h)

    def onReconfigure(self, device_id, configuration):
        """Set values in a device
        """
        props = ", ".join(configuration.keys())
        logger.info(f"Request to reconfigure the properties <b>{props}</b> "
                    f" of device <b>{device_id}</b>")
        h = Hash("type", "reconfigure")
        h["deviceId"] = device_id
        h["configuration"] = configuration
        h["reply"] = True
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        self._write_hash(h)

    def onInitDevice(self, server_id, class_id, device_id, config,
                     attrUpdates=None):
        h = Hash("type", "initDevice")
        h["serverId"] = server_id
        h["classId"] = class_id
        h["deviceId"] = device_id
        h["configuration"] = config
        if attrUpdates is not None:
            h["schemaUpdates"] = attrUpdates
        self._write_hash(h)

    def onExecute(self, device_id, slot_name, ignore_timeouts):
        logger.info(f"Executing slot <b>{slot_name}</b> "
                    f"of device <b>{device_id}</b>")
        h = Hash("type", "execute")
        h["deviceId"] = device_id
        h["command"] = slot_name
        h["reply"] = True
        if not ignore_timeouts:
            h["timeout"] = REQUEST_REPLY_TIMEOUT
        self._write_hash(h)

    def onExecuteGeneric(self, instanceId, slot_name, params):
        logger.info(f"Executing slot <b>{slot_name}</b> "
                    f"of device <b>{instanceId}</b>")
        h = Hash("type", "requestGeneric")
        h["instanceId"] = instanceId
        h["slot"] = slot_name
        h["args"] = params
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["replyType"] = "requestGeneric"
        self._write_hash(h)

    def onStartMonitoringDevice(self, device_id):
        h = Hash("type", "startMonitoringDevice")
        h["deviceId"] = device_id
        self._write_hash(h)

    def onStopMonitoringDevice(self, device_id):
        h = Hash("type", "stopMonitoringDevice")
        h["deviceId"] = device_id
        self._write_hash(h)

    def onGetClassSchema(self, server_id, class_id):
        h = Hash("type", "getClassSchema")
        h["serverId"] = server_id
        h["classId"] = class_id
        self._write_hash(h)

    def onGetDeviceSchema(self, device_id):
        h = Hash("type", "getDeviceSchema")
        h["deviceId"] = device_id
        self._write_hash(h)

    def onGetPropertyHistory(self, device_id, path, t0, t1, maxNumData):
        h = Hash("type", "getPropertyHistory")
        h["deviceId"] = device_id
        h["property"] = path
        h["t0"] = t0
        h["t1"] = t1
        h["maxNumData"] = maxNumData
        self._write_hash(h)

    # ---------------------------------------------------------------------
    # Current Configuration Interface

    def onGetConfigurationFromPast(self, device_id, time, preview):
        logger.info(f"Requesting configuration for device "
                    f"<b>{device_id}</b> at time point {time}")
        h = Hash("type", "getConfigurationFromPast")
        h["deviceId"] = device_id
        h["time"] = time
        h["preview"] = preview
        self._write_hash(h)

    def onGetConfigurationFromName(self, device_id, name, preview):
        logger.info(f"Requesting named configuration {name} "
                    f"for device <b>{device_id}</b>")
        h = Hash("type", "requestGeneric")
        args = Hash(
            "deviceId", device_id,
            "name", name)
        h["args"] = args
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["instanceId"] = KARABO_CONFIG_MANAGER
        h["preview"] = preview
        h["slot"] = "slotGetConfigurationFromName"
        h["replyType"] = "getConfigurationFromName"
        self._write_hash(h)

    def onListConfigurationFromName(self, device_id, name=""):
        logger.info("Requesting list of configuration from name for"
                    f"device <b>{device_id}</b>")
        h = Hash("type", "requestGeneric")
        args = Hash(
            "name", name,
            "deviceId", device_id)
        h["args"] = args
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["instanceId"] = KARABO_CONFIG_MANAGER
        h["slot"] = "slotListConfigurationFromName"
        h["replyType"] = "listConfigurationFromName"
        self._write_hash(h)

    def onSaveConfigurationFromName(self, name, deviceIds, description='',
                                    priority=1, update=False):
        logger.info(f"Saving configuration by name {name} for devices "
                    f"<b>{deviceIds}</b> with priority {priority}")
        h = Hash("type", "requestGeneric")
        args = Hash(
            "name", name,
            "deviceIds", deviceIds,
            "description", description,
            "priority", priority,
            "client", krb_globals.KARABO_CLIENT_ID)
        h["args"] = args
        h["update"] = update
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["instanceId"] = KARABO_CONFIG_MANAGER
        h["slot"] = "slotSaveConfigurationFromName"
        h["replyType"] = "saveConfigurationFromName"
        self._write_hash(h)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def onProjectBeginSession(self, project_manager):
        h = Hash("type", "projectBeginUserSession")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._write_hash(h)

    def onProjectEndSession(self, project_manager):
        h = Hash("type", "projectEndUserSession")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._write_hash(h)

    def onListProjectDomains(self, project_manager):
        h = Hash("type", "projectListDomains")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        self._write_hash(h)

    def onListProjectManagers(self):
        h = Hash("type", "projectListProjectManagers")
        h["token"] = get_config()["db_token"]
        self._write_hash(h)

    def onProjectListItems(self, project_manager, domain, item_type):
        h = Hash("type", "projectListItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["domain"] = domain
        h["item_types"] = [item_type]
        self._write_hash(h)

    def onProjectLoadItems(self, project_manager, items):
        h = Hash("type", "projectLoadItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["items"] = items
        self._write_hash(h)

    def onProjectSaveItems(self, project_manager, items):
        h = Hash("type", "projectSaveItems")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["items"] = items
        h["client"] = krb_globals.KARABO_CLIENT_ID
        self._write_hash(h)

    def onProjectUpdateAttribute(self, project_manager, items):
        h = Hash("type", "projectUpdateAttribute")
        h["projectManager"] = project_manager
        h["token"] = get_config()["db_token"]
        h["items"] = items
        self._write_hash(h)

    # ---------------------------------------------------------------------

    def onAcknowledgeAlarm(self, instanceId, rowId):
        h = Hash("type", "acknowledgeAlarm")
        h["alarmInstanceId"] = instanceId
        h["acknowledgedRows"] = Hash(rowId, True)
        self._write_hash(h)

    def onRequestAlarms(self, instanceId):
        h = Hash("type", "requestAlarms")
        h["alarmInstanceId"] = instanceId
        self._write_hash(h)

    def onSubscribeToOutput(self, device_id, path, subscribe):
        h = Hash("type", "subscribeNetwork")
        h["channelName"] = device_id + ":" + path
        h["subscribe"] = subscribe
        self._write_hash(h)

    def onRequestNetwork(self, name):
        h = Hash("type", "requestNetwork", "channelName", name)
        self._write_hash(h)

    def onError(self, error):
        h = Hash("type", "error", "traceback", error)
        self._write_hash(h)

    # --------------------------------------------------------------------------

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
        self._send_login_information()

    # --------------------------------------------------------------------------
    # private functions

    def _load_login_settings(self):
        """Load the login settings from the configuration singleton

        This method sets the default `host`, `port` and `gui_servers`
        """
        config = get_config()
        self.username = config['username']
        self.gui_servers = config['gui_servers']

        if self.gui_servers:
            self.hostname, self.port = self.gui_servers[0].split(':')

        self.port = int(self.port)

    def _write_hash(self, h):
        # There might be a connect to server in progress, but without success
        if (self._tcp_socket is None or
                self._tcp_socket.state() == QAbstractSocket.HostLookupState or
                self._tcp_socket.state() == QAbstractSocket.ConnectingState):
            # Save request for connection established
            self._request_queue.append(h)
            return

        stream = QByteArray()
        dataBytes = encodeBinary(h)
        stream.push_back(QByteArray(pack('I', len(dataBytes))))
        stream.push_back(dataBytes)
        self._tcp_socket.write(stream)

    def _performance_monitor(self, received_timestamp):
        diff = Timestamp().toTimestamp() - received_timestamp
        self.signalNetworkPerformance.emit(diff, self._show_proc_delay)

    def _send_login_information(self):
        login_info = Hash("type", "login")
        login_info["username"] = krb_globals.KARABO_CLIENT_ID
        login_info["version"] = krb_globals.GUI_VERSION
        self._write_hash(login_info)
