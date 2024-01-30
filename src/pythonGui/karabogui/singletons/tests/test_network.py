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
from unittest.mock import patch
from uuid import uuid4

from qtpy.QtNetwork import QAbstractSocket

from karabo.native import Hash
from karabogui.singletons.configuration import Configuration
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.network import Network
from karabogui.testing import GuiTestCase, singletons


class TestNetwork(GuiTestCase):

    def setUp(self):
        super().setUp()
        self._last_hash = None

    def test_member_variables(self):
        network = Network()
        with singletons(network=network):
            assert network.hostname == 'localhost'
            assert network.username == 'operator'
            assert network.password == 'karabo'
            assert network.port == 44444

    @patch('karabogui.singletons.network.QTcpSocket')
    def test_connect_directly(self, socket):
        network = Network()
        config = Configuration()
        with singletons(network=network, configuration=config):
            host = "xfel-computer-system-control"
            port = 67893
            username = "admin"
            success = network.connectToServerDirectly(
                username, host, port)
            assert success
            socket().connectToHost.assert_called_with(host, port)

            network.onConnected()
            assert config["username"] == username
            # And disconnect!
            network.disconnectFromServer()
            socket().disconnectFromHost.assert_called_once()

    def slotReceiveData(self, data):
        self._last_hash = data

    @patch('karabogui.singletons.network.QTcpSocket')
    def test_socket_connect_login_protocol(self, socket):
        network = Network()
        mediator = Mediator()
        network.signalReceivedData.connect(self.slotReceiveData)
        with singletons(network=network, mediator=mediator):
            host = "exfl-client-pc-system-guikarabo"
            port = 68580
            username = "admin"
            success = network.connectToServerDirectly(
                username, host, port)
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
            assert self._last_hash is not None
            assert isinstance(self._last_hash, Hash)
            assert self._last_hash["type"] == "login"

            with self.subTest("Test alarm interface"):
                instanceId = "test"
                network.onAcknowledgeAlarm(instanceId=instanceId, rowId="2")
                _trigger_message_parse()
                assert self._last_hash["type"] == "acknowledgeAlarm"
                assert self._last_hash["alarmInstanceId"] == instanceId
                assert self._last_hash["acknowledgedRows"] == Hash("2", True)

                network.onRequestAlarms(instanceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestAlarms"
                assert self._last_hash["alarmInstanceId"] == instanceId

            with self.subTest("Test misc network"):
                output_device = "NODEVICE/DESY/GLASPALACE"
                network.onSubscribeToOutput(output_device, "output", True)
                _trigger_message_parse()
                assert self._last_hash["type"] == "subscribeNetwork"
                assert self._last_hash[
                           "channelName"] == f"{output_device}:output"
                assert self._last_hash["subscribe"] is True

                network.onRequestNetwork(f"{output_device}:output")
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestNetwork"
                assert self._last_hash["channelName"] == \
                       f"{output_device}:output"

                network.onError("This is an error")
                _trigger_message_parse()
                assert self._last_hash["type"] == "error"
                assert self._last_hash["traceback"] == "This is an error"

            with self.subTest("Test Network Project Interface"):
                project_manager = "KaraboProjectDB"

                network.onProjectBeginSession(project_manager)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager

                network.onProjectEndSession(project_manager)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager

                network.onListProjectDomains(project_manager)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager

                network.onProjectListItems(project_manager, "FXE", "scene")
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager
                assert self._last_hash["args.domain"] == "FXE"
                assert self._last_hash["args.item_types"] == ["scene"]

                items = [str(uuid4()), str(uuid4())]
                network.onProjectLoadItems(project_manager, items)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager
                assert self._last_hash["args.items"] == items

                network.onProjectSaveItems(project_manager, items)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager
                assert self._last_hash["args.items"] == items
                assert self._last_hash["args.client"] is not None

                network.onProjectUpdateAttribute(project_manager, items)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == project_manager
                assert self._last_hash["args.items"] == items

            with self.subTest("Protocol Methods"):
                deviceId = "NoDeviceHere"
                network.onKillDevice(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "killDevice"
                assert self._last_hash["deviceId"] == deviceId

                serverId = "ProjectDBServer"
                network.onKillServer(serverId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "killServer"
                assert self._last_hash["serverId"] == serverId

                network.onGetDeviceConfiguration(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "getDeviceConfiguration"
                assert self._last_hash["deviceId"] == deviceId

                dev_config = Hash("position", 5)
                network.onReconfigure(deviceId, dev_config)
                _trigger_message_parse()
                assert self._last_hash["type"] == "reconfigure"
                assert self._last_hash["deviceId"] == deviceId
                assert self._last_hash["configuration"] == dev_config
                assert self._last_hash["reply"] is True
                assert self._last_hash["timeout"] == 5

                classId = "NoSpecialClassId"
                network.onInitDevice(serverId, classId, deviceId, dev_config)
                _trigger_message_parse()
                assert self._last_hash["type"] == "initDevice"
                assert self._last_hash["deviceId"] == deviceId
                assert self._last_hash["serverId"] == serverId
                assert self._last_hash["classId"] == classId
                assert self._last_hash["configuration"] == dev_config

                slotName = "start"
                network.onExecute(deviceId, slotName, False)
                _trigger_message_parse()
                assert self._last_hash["type"] == "execute"
                assert self._last_hash["deviceId"] == deviceId
                assert self._last_hash["command"] == slotName
                assert self._last_hash["reply"] is True
                assert self._last_hash["timeout"] == 5

                network.onExecuteGeneric(deviceId, slotName, dev_config)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == deviceId
                assert self._last_hash["slot"] == slotName
                assert self._last_hash["args"] == dev_config
                assert self._last_hash["timeout"] == 5
                assert self._last_hash["replyType"] == "requestGeneric"

                network.onStartMonitoringDevice(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "startMonitoringDevice"
                assert self._last_hash["deviceId"] == deviceId

                network.onStopMonitoringDevice(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "stopMonitoringDevice"
                assert self._last_hash["deviceId"] == deviceId

                network.onGetClassSchema(serverId, classId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "getClassSchema"
                assert self._last_hash["serverId"] == serverId
                assert self._last_hash["classId"] == classId

                network.onGetDeviceSchema(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "getDeviceSchema"
                assert self._last_hash["deviceId"] == deviceId

                path = "position"
                t0 = 0
                t1 = 1
                maxNumData = 500
                network.onGetPropertyHistory(deviceId, path, t0, t1,
                                             maxNumData)
                _trigger_message_parse()
                assert self._last_hash["type"] == "getPropertyHistory"
                assert self._last_hash["deviceId"] == deviceId
                assert self._last_hash["t0"] == t0
                assert self._last_hash["t1"] == t1
                assert self._last_hash["property"] == path
                assert self._last_hash["maxNumData"] == maxNumData

            with self.subTest("Configuration Interface"):
                preview = True
                network.onGetConfigurationFromPast(deviceId, t0, preview)
                _trigger_message_parse()
                assert self._last_hash["type"] == "getConfigurationFromPast"
                assert self._last_hash["deviceId"] == deviceId
                assert self._last_hash["time"] == t0
                assert self._last_hash["preview"] == preview

                conf_name = "test"
                network.onGetConfigurationFromName(deviceId, conf_name,
                                                   preview)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["args.deviceId"] == deviceId
                assert self._last_hash["timeout"] == 5
                assert self._last_hash["preview"] == preview
                assert self._last_hash[
                           "slot"] == "slotGetConfigurationFromName"
                assert self._last_hash[
                           "replyType"] == "getConfigurationFromName"

                network.onListConfigurationFromName(deviceId)
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["args.deviceId"] == deviceId
                assert self._last_hash["args.name"] == ""
                assert self._last_hash[
                           "slot"] == "slotListConfigurationFromName"
                assert self._last_hash[
                           "replyType"] == "listConfigurationFromName"

                network.onSaveConfigurationFromName(conf_name, [deviceId])
                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["args.deviceIds"] == [deviceId]
                assert self._last_hash["args.name"] == conf_name
                assert self._last_hash[
                           "slot"] == "slotSaveConfigurationFromName"
                assert self._last_hash[
                           "replyType"] == "saveConfigurationFromName"
                assert self._last_hash["timeout"] == 5

            with self.subTest("Log messages"):
                network.onSetLogPriority("swerver", "ERROR")
                _trigger_message_parse()
                assert self._last_hash["type"] == "setLogPriority"
                assert self._last_hash["instanceId"] == "swerver"
                assert self._last_hash["priority"] == "ERROR"

            with patch('karabogui.singletons.network.QMessageBox') as mbox, \
                    patch('karabogui.singletons.network.LoginDialog') as dia:
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

            with self.subTest("Active Proposals"):
                network.listDestinations()
                _trigger_message_parse()
                h = Hash("type", "requestGeneric",
                         "instanceId", "KaraboLogBook",
                         "slot", "slotListDestinations",
                         "replyType", "listDestinations",
                         "args", Hash(),
                         )
                assert self._last_hash.fullyEqual(h)

            with self.subTest("SaveLogBook"):
                name = "TestProposal"
                dataType = "Image"
                data = "Test data"
                caption = "This is a test message"
                network.onSaveLogBook(
                    name=name, dataType=dataType, data=data,
                    caption=caption)

                _trigger_message_parse()
                assert self._last_hash["type"] == "requestGeneric"
                assert self._last_hash["instanceId"] == "KaraboLogBook"
                assert self._last_hash["args.name"] == name
                assert self._last_hash["args.dataType"] == dataType
