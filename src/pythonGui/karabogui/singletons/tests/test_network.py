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
from uuid import uuid4

from qtpy.QtNetwork import QAbstractSocket
from qtpy.QtWidgets import QDialog

import karabogui.access as krb_access
from karabo.native import Hash
from karabogui.singletons.configuration import Configuration
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.network import Network
from karabogui.testing import singletons


class MockReceiver:
    def __init__(self):
        self.last_hash = None
        self.server_connection = False

    def slotReceiveData(self, data):
        self.last_hash = data

    def slotServerConnection(self, status):
        self.server_connection = status


def test_member_variables(gui_app):
    network = Network()
    with singletons(network=network):
        assert network.hostname == 'localhost'
        assert network.password == 'karabo'
        assert network.port == 44444


def test_connect_directly(mocker, gui_app):
    socket = mocker.patch('karabogui.singletons.network.QTcpSocket')
    network = Network()
    config = Configuration()
    with singletons(network=network, configuration=config):
        host = "xfel-computer-system-control"
        port = 33333
        mock_login_dialog = mocker.patch(
            "karabogui.singletons.network.ReactiveLoginDialog")
        mock_login_dialog().hostname = host
        mock_login_dialog().port = port
        mock_login_dialog().gui_servers = []
        mock_login_dialog().access_level = "operator"
        mock_login_dialog().exec.return_value = QDialog.Accepted
        success = network.connectToServerDirectly(host, port)
        assert success
        socket().connectToHost.assert_called_with(host, port)

        network.onConnected()
        # And disconnect!
        network.disconnectFromServer()
        socket().disconnectFromHost.assert_called_once()


def test_socket_connect_login_protocol(mocker, subtests, gui_app):
    socket = mocker.patch('karabogui.singletons.network.QTcpSocket')
    network = Network()
    mediator = Mediator()
    receiver = MockReceiver()
    network.signalReceivedData.connect(receiver.slotReceiveData)
    network.signalServerConnectionChanged.connect(
        receiver.slotServerConnection)
    with singletons(network=network, mediator=mediator):
        host = "exfl-client-pc-system-guikarabo"
        port = 32323
        mock_login_dialog = mocker.patch(
            "karabogui.singletons.network.ReactiveLoginDialog")
        mock_login_dialog().hostname = host
        mock_login_dialog().port = port
        mock_login_dialog().gui_servers = []
        mock_login_dialog().access_level = "operator"

        mock_login_dialog().exec.return_value = QDialog.Accepted
        success = network.connectToServerDirectly(host, port)
        assert success
        socket().connectToHost.assert_called_with(host, port)

        # We received reply, we set server information and connect
        # send our login information!
        network.set_server_information(read_only=False)

        def _trigger_message_parse():
            nonlocal socket
            # Get the byte array and remove size
            call = socket().write.call_args[0]
            byte_array = call[0]
            byte_array = byte_array[4:]
            data = byte_array.data()
            # Place timestamp to await and parse Input to trigger signal
            network._waiting_messages[id(data)] = 0
            network.parseInput(data)

        _trigger_message_parse()
        assert receiver.last_hash is not None
        assert isinstance(receiver.last_hash, Hash)
        assert receiver.last_hash["type"] == "login"

        with subtests.test(msg="Test alarm interface"):
            instanceId = "test"
            network.onAcknowledgeAlarm(instanceId=instanceId, rowId="2")
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "acknowledgeAlarm"
            assert receiver.last_hash["alarmInstanceId"] == instanceId
            assert receiver.last_hash["acknowledgedRows"] == Hash("2", True)

            network.onRequestAlarms(instanceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestAlarms"
            assert receiver.last_hash["alarmInstanceId"] == instanceId

        with subtests.test(msg="Test misc network"):
            output_device = "NODEVICE/DESY/GLASPALACE"
            network.onSubscribeToOutput(output_device, "output", True)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "subscribeNetwork"
            assert receiver.last_hash[
                       "channelName"] == f"{output_device}:output"
            assert receiver.last_hash["subscribe"] is True

            network.onRequestNetwork(f"{output_device}:output")
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestNetwork"
            assert receiver.last_hash["channelName"] == \
                   f"{output_device}:output"

            text = "This is an error"
            network.onInfo(Hash("traceback", text, "type", "error"))
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "error"
            assert receiver.last_hash["info"]["traceback"] == text

        with subtests.test(msg="Test Network Project Interface"):
            project_manager = "KaraboProjectDB"

            network.onProjectBeginSession(project_manager)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager

            network.onProjectEndSession(project_manager)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager

            network.onListProjectDomains(project_manager)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager

            network.onProjectListItems(project_manager, "FXE", "scene")
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager
            assert receiver.last_hash["args.domain"] == "FXE"
            assert receiver.last_hash["args.item_types"] == ["scene"]

            items = [str(uuid4()), str(uuid4())]
            network.onProjectLoadItems(project_manager, items)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager
            assert receiver.last_hash["args.items"] == items

            network.onProjectSaveItems(project_manager, items)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager
            assert receiver.last_hash["args.items"] == items
            assert receiver.last_hash["args.client"] is not None

            network.onProjectUpdateAttribute(project_manager, items)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == project_manager
            assert receiver.last_hash["args.items"] == items

        with subtests.test(msg="Protocol Methods"):
            deviceId = "NoDeviceHere"
            network.onKillDevice(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "killDevice"
            assert receiver.last_hash["deviceId"] == deviceId

            serverId = "ProjectDBServer"
            network.onKillServer(serverId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "killServer"
            assert receiver.last_hash["serverId"] == serverId

            network.onGetDeviceConfiguration(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "getDeviceConfiguration"
            assert receiver.last_hash["deviceId"] == deviceId

            dev_config = Hash("position", 5)
            network.onReconfigure(deviceId, dev_config)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "reconfigure"
            assert receiver.last_hash["deviceId"] == deviceId
            assert receiver.last_hash["configuration"] == dev_config
            assert receiver.last_hash["reply"] is True
            assert receiver.last_hash["timeout"] == 5

            classId = "NoSpecialClassId"
            network.onInitDevice(serverId, classId, deviceId, dev_config)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "initDevice"
            assert receiver.last_hash["deviceId"] == deviceId
            assert receiver.last_hash["serverId"] == serverId
            assert receiver.last_hash["classId"] == classId
            assert receiver.last_hash["configuration"] == dev_config

            slotName = "start"
            network.onExecute(deviceId, slotName, False)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "execute"
            assert receiver.last_hash["deviceId"] == deviceId
            assert receiver.last_hash["command"] == slotName
            assert receiver.last_hash["reply"] is True
            assert receiver.last_hash["timeout"] == 5

            network.onExecuteGeneric(deviceId, slotName, dev_config)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == deviceId
            assert receiver.last_hash["slot"] == slotName
            assert receiver.last_hash["args"] == dev_config
            assert receiver.last_hash["timeout"] == 5
            assert receiver.last_hash["replyType"] == "requestGeneric"

            network.onStartMonitoringDevice(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "startMonitoringDevice"
            assert receiver.last_hash["deviceId"] == deviceId

            network.onStopMonitoringDevice(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "stopMonitoringDevice"
            assert receiver.last_hash["deviceId"] == deviceId

            network.onGetClassSchema(serverId, classId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "getClassSchema"
            assert receiver.last_hash["serverId"] == serverId
            assert receiver.last_hash["classId"] == classId

            network.onGetDeviceSchema(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "getDeviceSchema"
            assert receiver.last_hash["deviceId"] == deviceId

            path = "position"
            t0 = 0
            t1 = 1
            maxNumData = 500
            network.onGetPropertyHistory(deviceId, path, t0, t1,
                                         maxNumData)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "getPropertyHistory"
            assert receiver.last_hash["deviceId"] == deviceId
            assert receiver.last_hash["t0"] == t0
            assert receiver.last_hash["t1"] == t1
            assert receiver.last_hash["property"] == path
            assert receiver.last_hash["maxNumData"] == maxNumData

        with subtests.test(msg="Configuration Interface"):
            preview = True
            network.onGetConfigurationFromPast(deviceId, t0, preview)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "getConfigurationFromPast"
            assert receiver.last_hash["deviceId"] == deviceId
            assert receiver.last_hash["time"] == t0
            assert receiver.last_hash["preview"] == preview

            conf_name = "test"
            network.onGetConfigurationFromName(deviceId, conf_name,
                                               preview)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["args.deviceId"] == deviceId
            assert receiver.last_hash["timeout"] == 5
            assert receiver.last_hash["preview"] == preview
            assert receiver.last_hash[
                       "slot"] == "slotGetConfigurationFromName"
            assert receiver.last_hash[
                       "replyType"] == "getConfigurationFromName"

            network.onListConfigurationFromName(deviceId)
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["args.deviceId"] == deviceId
            assert receiver.last_hash["args.name"] == ""
            assert receiver.last_hash[
                       "slot"] == "slotListConfigurationFromName"
            assert receiver.last_hash[
                       "replyType"] == "listConfigurationFromName"

            network.onSaveConfigurationFromName(conf_name, [deviceId])
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["args.deviceIds"] == [deviceId]
            assert receiver.last_hash["args.name"] == conf_name
            assert receiver.last_hash[
                       "slot"] == "slotSaveConfigurationFromName"
            assert receiver.last_hash[
                       "replyType"] == "saveConfigurationFromName"
            assert receiver.last_hash["timeout"] == 5

        with subtests.test(msg="Log messages"):
            network.onSetLogPriority("swerver", "ERROR")
            _trigger_message_parse()
            assert receiver.last_hash["type"] == "setLogPriority"
            assert receiver.last_hash["instanceId"] == "swerver"
            assert receiver.last_hash["priority"] == "ERROR"

        mbox = mocker.patch('karabogui.singletons.network.QMessageBox')
        dia = mocker.patch('karabogui.singletons.network.ReactiveLoginDialog')
        call_count = 0
        for error in [QAbstractSocket.ConnectionRefusedError,
                      QAbstractSocket.RemoteHostClosedError,
                      QAbstractSocket.HostNotFoundError,
                      QAbstractSocket.NetworkError,
                      QAbstractSocket.SocketAccessError,
                      QAbstractSocket.DatagramTooLargeError]:
            # Last error not in list of expected !
            network.onSocketError(error)
            call_count += 1
            mbox.question.call_count = call_count
            # We are asked to relogin on error
            dia.call_count = call_count

        with subtests.test(msg="Active Proposals"):
            network.listDestinations()
            _trigger_message_parse()
            h = Hash("type", "requestGeneric",
                     "instanceId", "KaraboLogBook",
                     "slot", "slotListDestinations",
                     "replyType", "listDestinations",
                     "args", Hash(),
                     )
            assert receiver.last_hash.fullyEqual(h)

        with subtests.test(msg="SaveLogBook"):
            name = "TestProposal"
            dataType = "Image"
            data = "Test data"
            caption = "This is a test message"
            network.onSaveLogBook(
                name=name, dataType=dataType, data=data,
                caption=caption)

            _trigger_message_parse()
            assert receiver.last_hash["type"] == "requestGeneric"
            assert receiver.last_hash["instanceId"] == "KaraboLogBook"
            assert receiver.last_hash["args.name"] == name
            assert receiver.last_hash["args.dataType"] == dataType

        with subtests.test(msg="Network connection"):
            assert receiver.server_connection is False
            network.onConnected()
            assert receiver.server_connection is True
            krb_access.ONE_TIME_TOKEN = 123345
            krb_access.TEMPORARY_SESSION_USER = "karabo"
            krb_access.TEMPORARY_SESSION_WARNING = True
            network.disconnectFromServer()
            assert receiver.server_connection is False
            assert krb_access.ONE_TIME_TOKEN is None
            assert krb_access.TEMPORARY_SESSION_USER is None
            assert krb_access.TEMPORARY_SESSION_WARNING is False
