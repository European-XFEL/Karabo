# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import getpass
from functools import partial
from struct import calcsize, pack, unpack

from qtpy.QtCore import QByteArray, QObject, Signal, Slot
from qtpy.QtNetwork import QAbstractSocket, QTcpSocket
from qtpy.QtWidgets import QDialog, QMessageBox, qApp

import karabogui.access as krb_access
from karabo.common.api import (
    KARABO_CONFIG_MANAGER, KARABO_LOGBOOK_MANAGER, KARABO_PROJECT_MANAGER)
from karabo.native import (
    AccessLevel, Hash, Timestamp, decodeBinary, dictToHash, encodeBinary)
from karabogui import background, const
from karabogui.const import REQUEST_REPLY_TIMEOUT
from karabogui.dialogs.reactive_login_dialog import ReactiveLoginDialog
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.logger import get_logger
from karabogui.singletons.api import get_config
from karabogui.util import process_qt_events

MAX_GUI_SERVER_HISTORY = 5
PROJECT_DB_TIMEOUT = 10

logger = get_logger()


class Network(QObject):
    """The `Network` class is the singleton holding the tcp socket for
    the gui server connection.
    """
    signalServerConnectionChanged = Signal(bool)
    signalReceivedData = Signal(object)
    signalNetworkPerformance = Signal(float, bool)

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        self._tcp_socket = None
        self._data_reader = None
        self._request_queue = []

        self._waiting_messages = {}
        self._show_proc_delay = False

        self.gui_servers = []
        self.hostname = "localhost"
        self.port = "44444"
        self.password = "karabo"
        self.access_level = "observer"

        # Check default settings stored in QSettings!
        self._load_login_settings()

        # Listen for the quit notification
        qApp.aboutToQuit.connect(self.onQuitApplication)

        if get_config()["development"]:
            self.togglePerformanceMonitor()

    def connectToServer(self, parent=None) -> bool:
        """Connection to server via reactive LoginDialog.

        Returns: Boolean to indicate if the login dialog has been accepted
        and that the user has been successfully authenticated.
        """
        dialog = ReactiveLoginDialog(
            access_level=self.access_level,
            hostname=self.hostname,
            port=self.port,
            gui_servers=self.gui_servers,
            parent=parent)

        if dialog.exec() == QDialog.Accepted:
            self.hostname = dialog.hostname
            self.port = dialog.port
            self.gui_servers = dialog.gui_servers
            self.access_level = dialog.access_level
            self.startServerConnection()
            return True

        # Note: The server connection is not changed and thus we must
        # notify the main window to untoggle the button
        self.signalServerConnectionChanged.emit(False)

        return False

    def connectToServerDirectly(self, hostname, port):
        """Connection to server directly via username, host and port

        This function is used for fast startup via the karabo cinema
        """
        self.hostname = hostname
        self.port = port
        dialog = ReactiveLoginDialog(hostname=hostname,
                                     gui_servers=self.gui_servers,
                                     port=port)
        if dialog.exec() != QDialog.Accepted:
            return False
        self.hostname = dialog.hostname
        self.port = dialog.port
        self.gui_servers = dialog.gui_servers
        self.access_level = dialog.access_level
        self.startServerConnection()
        # Allow external runner to see the status of the connection!
        return True

    def disconnectFromServer(self):
        """Disconnect from server"""
        krb_access.reset_login()
        self.signalServerConnectionChanged.emit(False)
        # All panels need to be reset and all projects closed
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
            get_logger().info(
                "Disconnected from the gui server "
                f"<b>{self.hostname}:{self.port}</b>")
            return

        get_logger().error(
            f"Disconnect failed: {self._tcp_socket.errorString()}")

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
        server = f'{self.hostname}:{self.port}'

        self.gui_servers = _least_recently_used(server, self.gui_servers,
                                                int(MAX_GUI_SERVER_HISTORY))

        # Save to singleton!
        get_config()['access_level'] = self.access_level
        get_config()['gui_servers'] = self.gui_servers
        self._data_reader = self._network_generator()

        # Update MainWindow toolbar
        self.signalServerConnectionChanged.emit(True)

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
        logger.info(f"Requesting to instantiate device <b>{device_id}</b> "
                    f"with <b>{class_id}</b> on server <b>{server_id}</b>")
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

    def onExecuteGeneric(self, instanceId, slot_name, params, token=None):
        logger.info(f"Executing slot <b>{slot_name}</b> "
                    f"of instance <b>{instanceId}</b>")
        h = Hash("type", "requestGeneric")
        if token is not None:
            h["token"] = token
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
        logger.info(f"Requesting named configuration <b>{name}</b> "
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
        logger.info("Requesting list of named configurations for "
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

    def onDeleteConfigurationFromName(self, device_id, name):
        logger.info("Requesting delete of named configurations for "
                    f"device <b>{device_id}:{name}</b>")
        h = Hash("type", "requestGeneric")
        args = Hash(
            "name", name,
            "deviceId", device_id)
        h["args"] = args
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["instanceId"] = KARABO_CONFIG_MANAGER
        h["slot"] = "slotDeleteConfiguration"
        h["replyType"] = "deleteConfigurationFromName"
        self._write_hash(h)

    def onSaveConfigurationFromName(self, name, deviceIds, update=False):
        logger.info(f"Saving configuration by name {name} for devices "
                    f"<b>{deviceIds}</b>")
        h = Hash("type", "requestGeneric")
        args = Hash(
            "name", name,
            "deviceIds", deviceIds,
            "client", const.KARABO_CLIENT_ID)
        h["args"] = args
        h["update"] = update
        h["timeout"] = REQUEST_REPLY_TIMEOUT
        h["instanceId"] = KARABO_CONFIG_MANAGER
        h["slot"] = "slotSaveConfigurationFromName"
        h["replyType"] = "saveConfigurationFromName"
        self._write_hash(h)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def _request_project_message(self, action_type, reply_type, timeout=None,
                                 **kwargs):
        h = Hash("type", "requestGeneric")
        args = Hash(
            "token", get_config()["db_token"],
            "type", action_type)
        for key, value in kwargs.items():
            args[key] = value
        h["args"] = args
        h["empty"] = True
        h["timeout"] = timeout or PROJECT_DB_TIMEOUT
        h["instanceId"] = KARABO_PROJECT_MANAGER
        h["slot"] = "slotGenericRequest"
        h["replyType"] = reply_type
        self._write_hash(h)

    def onProjectBeginSession(self, project_manager):
        self._request_project_message("beginUserSession",
                                      "projectBeginUserSession")

    def onProjectEndSession(self, project_manager):
        self._request_project_message("endUserSession",
                                      "projectEndUserSession")

    def onListProjectDomains(self, project_manager):
        self._request_project_message(
            "listDomains", "projectListDomains")

    def onProjectListItems(self, project_manager, domain, item_type):
        self._request_project_message("listItems", "projectListItems",
                                      domain=domain, item_types=[item_type])

    def onProjectListProjectsWithDevice(self, project_manager,
                                        domain, device_id):
        self._request_project_message(
            "listProjectsWithDevice", "projectListProjectsWithItem",
            timeout=120, name=device_id, domain=domain)

    def onProjectListProjectsWithMacro(self, project_manager,
                                       domain, macro_id):
        self._request_project_message(
            "listProjectsWithMacro", "projectListProjectsWithItem",
            timeout=120, name=macro_id, domain=domain)

    def onProjectListProjectsWithServer(self, project_manager,
                                        domain, server_id):
        self._request_project_message(
            "listProjectsWithServer", "projectListProjectsWithItem",
            timeout=120, name=server_id, domain=domain)

    def onProjectLoadItems(self, project_manager, items):
        self._request_project_message(
            "loadItems", "projectLoadItems", timeout=30, items=items)

    def onProjectSaveItems(self, project_manager, items):
        self._request_project_message(
            "saveItems", "projectSaveItems", timeout=30,
            client=const.KARABO_CLIENT_ID, items=items)

    def onProjectUpdateAttribute(self, project_manager, items):
        self._request_project_message("updateAttribute",
                                      "projectUpdateAttribute", items=items)

    # ---------------------------------------------------------------------

    def onAcknowledgeAlarm(self, instanceId, rowId):
        logger.info(f"Acknowledging alarm with ID <b>{rowId}</b> "
                    f"of alarm service <b>{instanceId}</b>")
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

    def onInfo(self, info: Hash):
        # XXX: Change `type` to `info` in Karabo 3
        assert isinstance(info, Hash)
        h = Hash("type", "error", "info", info,
                 "clientId", const.KARABO_CLIENT_ID)
        self._write_hash(h)

    def onSetLogLevel(self, instanceId, level):
        h = Hash("type", "setLogLevel", "instanceId", instanceId,
                 "level", level)
        self._write_hash(h)

    # ------------------------------------------------------------------------

    def set_server_information(self, read_only=False, **kwargs):
        """We get the reply from the GUI Server and set the information"""
        if read_only:
            default = AccessLevel.OBSERVER
        else:
            default = AccessLevel(
                krb_access.ACCESS_LEVEL_MAP.get(self.access_level,
                                                AccessLevel.EXPERT))

        krb_access.GLOBAL_ACCESS_LEVEL = default
        # Inform the GUI to change correspondingly the allowed
        # level-downgrade
        broadcast_event(KaraboEvent.LoginUserChanged, {})
        self.onLogin()
        self._empty_request_queue()

    # ------------------------------------------------------------------------
    # private functions

    def _load_login_settings(self):
        """Load the login settings from the configuration singleton

        This method sets the default `host`, `port` and `gui_servers`
        """
        config = get_config()
        self.access_level = config['access_level']
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

    def onLogin(self):
        login_info = Hash("type", "login")
        # username to transport ClientID is deprecated - remove it after a
        # couple of new versions of the GUI Server.
        login_info["username"] = const.KARABO_CLIENT_ID
        # Redundantly send, during the above key deprecation period, the
        # the ClientID with its new key.
        login_info["clientId"] = const.KARABO_CLIENT_ID
        login_info["version"] = const.GUI_VERSION
        login_info["applicationMode"] = const.APPLICATION_MODE
        if krb_access.is_authenticated():
            login_info["oneTimeToken"] = krb_access.ONE_TIME_TOKEN
            # For authenticated logins, don't set the "clientUserId" - the
            # authenticated userId and the authorized AccessLevel will
            # be returned later as a result of the token validation.
        else:
            # Non authenticated logins:
            # Send the OS id of the user running the process to the server for
            # logging/auditing purposes.
            login_info["clientUserId"] = getpass.getuser()

        login_info["info"] = dictToHash(get_config().info())
        self._write_hash(login_info)

    def beginTemporarySession(self, **info):
        h = Hash("type", "beginTemporarySession")
        h["clientId"] = const.KARABO_CLIENT_ID
        h["version"] = const.GUI_VERSION
        h["temporarySessionToken"] = info["temporarySessionToken"]
        h["levelBeforeTemporarySession"] = info["levelBeforeTemporarySession"]
        self._write_hash(h)

    def endTemporarySession(self, **info):
        h = Hash("type", "endTemporarySession")
        h["temporarySessionToken"] = krb_access.ONE_TIME_TOKEN
        self._write_hash(h)

    def _empty_request_queue(self):
        """If some requests got piled up, because of no server connection,
        now these get handled after the login"""
        for r in self._request_queue:
            self._write_hash(r)
        self._request_queue = []

    def listDestinations(self):
        logger.info("Fetching available active proposals in the Topic.")
        h = Hash("type", "requestGeneric")
        h["instanceId"] = KARABO_LOGBOOK_MANAGER
        h["slot"] = "slotListDestinations"
        h["replyType"] = "listDestinations"
        h["args"] = Hash()
        self._write_hash(h)

    def onSaveLogBook(self, **info):
        h = Hash()
        h["type"] = "requestGeneric"
        h["args"] = Hash(info)
        h["instanceId"] = KARABO_LOGBOOK_MANAGER
        h["slot"] = "slotSaveLogBook"
        h["replyType"] = "saveLogBook"
        self._write_hash(h)
