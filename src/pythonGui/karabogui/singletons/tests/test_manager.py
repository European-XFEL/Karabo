from inspect import signature
from unittest.mock import ANY, Mock, call, patch

from qtpy.QtCore import QSize

from karabo.common.api import WeakMethodRef
from karabo.native import (
    AccessLevel, AccessMode, Configurable, Hash, Int32, Schema, Timestamp)
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, ProjectDeviceProxy,
    ProxyStatus, build_binding)
from karabogui.events import KaraboEvent
from karabogui.testing import GuiTestCase, alarm_data, singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

from ..manager import Manager, project_db_handler
from ..mediator import Mediator

TEST_SERVER_ID = 'swerver'
TEST_CLASS_ID = 'PrettyDevice'
TEST_DEVICE_ID = 'dev'


def make_project_db_handler(fall_through):
    # Creates a wrapped function to test the project_db_handler decorator
    @project_db_handler(fall_through)
    def handle(self, success, request, reply, reason=''):
        return reply

    return handle


class PrettyDevice(Configurable):
    init_prop = Int32(accessMode=AccessMode.INITONLY)
    ro_prop = Int32(accessMode=AccessMode.READONLY)


def _get_class_proxy(schema):
    binding = build_binding(schema)
    return DeviceClassProxy(binding=binding)


def _get_project_device_proxy(server_id, class_id, device_id):
    with singletons(topology=SystemTopology()):
        binding = BindingRoot(class_id=class_id)
        proxy = ProjectDeviceProxy(device_id=device_id,
                                   server_id=server_id,
                                   binding=binding)
        build_binding(PrettyDevice.getClassSchema(), existing=proxy.binding)
    return proxy


class TestManager(GuiTestCase):
    def test_shutdown_device(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            manager.shutdownDevice('dev', showConfirm=False)
            network.onKillDevice.assert_called_with('dev')
            network.reset_mock()
            manager.shutdownDevice('dev', showConfirm=False)
            with patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_called_with('dev')
                network.onKillDevice.reset_mock()
                network.reset_mock()
                mb.reset_mock()
                manager.shutdownDevice('dev', showConfirm=False)
                network.onKillDevice.assert_called_with('dev')
                network.onKillDevice.reset_mock()
                mb().exec.return_value = mb.No
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_not_called()

            with patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                network.reset_mock()
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_called_with('dev')

                mb().exec.return_value = mb.No
                network.reset_mock()
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_not_called()

    def test_shutdown_server(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            with patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                manager.shutdownServer('swerver')
                network.onKillServer.assert_called_with('swerver')
                network.onKillServer.reset_mock()
                mb.reset_mock()
                mb().exec.return_value = mb.No
                manager.shutdownServer('swerver')
                network.onKillServer.assert_not_called()

                mb().exec.return_value = mb.No
                manager.shutdownServer('swerver')
                network.onKillServer.assert_not_called()

    def test_call_device_slot(self):
        handler_args = None

        def handler(success, reply):
            nonlocal handler_args
            handler_args = success, reply

        def request_handler(success, reply, request):
            nonlocal handler_args
            handler_args = success, reply, request

        params = Hash('arg0', 0, 'arg1', 1)
        reply = Hash('result', 'yer dumb')

        network = Mock()
        with singletons(network=network):
            manager = Manager()

            # 1. Call the slot of the 'remote device'
            token = manager.callDeviceSlot(handler, 'dev', 'slot', params)

            assert token in manager._request_handlers
            network.onExecuteGeneric.assert_called_with(
                'dev', 'slot', params, token=token)

            # Pretend the reply arrived
            manager.handle_requestGeneric(success=True,
                                          request=Hash("token", token),
                                          reply=reply)
            assert handler_args == (True, reply)

            # 2. Different signature, we receive request
            token = manager.callDeviceSlot(request_handler,
                                           'dev', 'slot', params)

            assert token in manager._request_handlers
            network.onExecuteGeneric.assert_called_with(
                'dev', 'slot', params, token=token)

            manager.handle_requestGeneric(success=True,
                                          request=Hash("token", token),
                                          reply=reply)
            assert handler_args == (True, reply, Hash("token", token))

            # 3. Test with WeakMethod

            class TestObject():
                def handler(self, success, reply, request):
                    nonlocal handler_args
                    handler_args = success, reply, request

            obj = TestObject()
            weak_handler = WeakMethodRef(obj.handler, num_args=3)
            sig = signature(weak_handler)
            assert len(sig.parameters) == 3

            token = manager.callDeviceSlot(weak_handler,
                                           'dev', 'slot', params)

            assert token in manager._request_handlers
            network.onExecuteGeneric.assert_called_with(
                'dev', 'slot', params, token=token)
            manager.handle_requestGeneric(success=True,
                                          request=Hash("token", token),
                                          reply=reply)
            assert handler_args == (True, reply, Hash("token", token))

    def test_handle_system_topology(self):
        topo_hash = system_hash()
        topology = Mock()
        mediator = Mediator()
        with singletons(topology=topology, mediator=mediator):
            manager = Manager()
            manager.handle_systemTopology(topo_hash)
            topology.initialize.assert_called_with(topo_hash)

    def test_handle_instance_new(self):
        network = Mock()
        topology = SystemTopology()
        topo_hash = system_hash()
        topology.initialize(topo_hash)
        with singletons(network=network, topology=topology):
            manager = Manager()
            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                topo_hash = system_hash()
                del topo_hash['device.divvy']
                del topo_hash['macro']

                changes = Hash("new", topo_hash,
                               "update", Hash(), "gone", Hash())
                manager.handle_topologyUpdate(changes)

                topo_update = {
                    'devices': [('orphan', 'Parentless',
                                 ProxyStatus.ONLINE)],
                    'servers': [('swerver', 'BIG_IRON', ProxyStatus.ONLINE)]
                }
                calls = broadcast_event.mock_calls
                assert len(calls) == 2
                # First call is to check for new devices
                assert calls[0][1] == (KaraboEvent.SystemTopologyUpdate,
                                       topo_update)
                # Second call is to clear with gone devices
                assert calls[1][1] == (KaraboEvent.ClearConfigurator,
                                       {'devices': []})

    def test_handle_instance_gone(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()

            devices = [('orphan', 'Parentless', ProxyStatus.OFFLINE)]
            topology.topology_update.return_value = (devices, [])

            calls = [
                call(KaraboEvent.SystemTopologyUpdate,
                     {'devices': devices, 'servers': []}),
                call(KaraboEvent.ClearConfigurator,
                     {'devices': ['orphan']})
            ]

            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                changes = Hash("new", Hash(),
                               "update", Hash(),
                               "gone", Hash("device", "orphan"))
                manager.handle_topologyUpdate(changes)
                broadcast_event.assert_has_calls(calls)

    def test_handle_instance_updated(self):
        topology = Mock()
        mediator = Mediator()
        with singletons(topology=topology, mediator=mediator):
            topology.topology_update.return_value = [], []
            manager = Manager()
            changes = Hash("new", Hash(), "update", Hash(),
                           "gone", Hash())
            manager.handle_topologyUpdate(changes)
            topology.topology_update.assert_called_with(changes)

    def test_handle_class_schema(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()

            schema = Schema()
            manager.handle_classSchema('server', 'ClassName', schema)

            topology.class_schema_updated.assert_called_with(
                'server', 'ClassName', schema)

    def test_handle_device_schema(self):
        network, topology = Mock(), Mock()
        mediator = Mediator()
        with singletons(network=network, topology=topology, mediator=mediator):
            manager = Manager()

            schema = Schema()
            reply = Hash('instanceId', 'dev', 'updatedSchema', schema)
            manager.handle_attributesUpdated(reply)
            topology.device_schema_updated.assert_called_with('dev', schema)

            manager.handle_deviceSchema('dev', schema)
            topology.device_schema_updated.assert_called_with('dev', schema)

    def test_init_device_none(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            # instantiate with wrong class def
            manager = Manager()
            topology.get_project_device_proxy.return_value = None

            cfg = Hash('init_prop', 42, 'ro_prop', -1)
            manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID,
                               config=cfg)
            # No proxy means, not called!
            network.onInitDevice.assert_not_called()

    def test_init_device_schema_evolution(self):
        network = Mock()
        topology = Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            schema = PrettyDevice.getClassSchema()
            topology.get_schema.return_value = schema
            proxy = _get_project_device_proxy(TEST_SERVER_ID, TEST_CLASS_ID,
                                              TEST_DEVICE_ID)

            topology.get_project_device_proxy.return_value = proxy

            cfg = Hash('init_prop', 42, 'ro_prop', -1, 'evolved', 43)
            manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID,
                               config=cfg)
            topology.get_schema.assert_called_with(TEST_SERVER_ID,
                                                   TEST_CLASS_ID)
            network.onInitDevice.assert_called_with(
                TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID,
                # The configuration will be stripped of all the keys
                # that are read-only or not in the schema
                Hash('init_prop', 42), attrUpdates=None
            )

    def test_init_device_empty_config(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            schema = PrettyDevice.getClassSchema()
            topology.get_schema.return_value = schema
            proxy = _get_project_device_proxy(TEST_SERVER_ID, TEST_CLASS_ID,
                                              TEST_DEVICE_ID)
            topology.get_project_device_proxy.return_value = proxy

            manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID)

            topology.get_schema.assert_called_with(TEST_SERVER_ID,
                                                   TEST_CLASS_ID)
            network.onInitDevice.assert_called_with(
                TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID,
                Hash(), attrUpdates=None)

    def test_init_device_badschema(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            topology.get_schema.return_value = None
            proxy = _get_project_device_proxy(TEST_SERVER_ID, TEST_CLASS_ID,
                                              TEST_DEVICE_ID)
            topology.get_project_device_proxy.return_value = proxy

            target = 'karabogui.messagebox.show_error'
            with patch(target) as msg_box:
                cfg = Hash('init_prop', 42, 'ro_prop', -1)
                manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID,
                                   TEST_DEVICE_ID, config=cfg)
                assert msg_box.call_count == 1

    def test_project_db_handler(self):
        bad_result = Hash('success', False, 'value', 1, 'reason', 'error_msg')
        good_result = Hash('success', True, 'value', 1)
        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as msg_box:
            # this creates a function wrapped by the handler that returns
            # the input data
            request = Hash()
            handler = make_project_db_handler(False)
            handler('placeholder', True, request, bad_result, reason="")
            # error shown in case of bad result
            msg_box.show_error.assert_called_with('error_msg', details=None)

            handled_result = handler('placeholder', True, request,
                                     good_result, reason="")
            assert handled_result.get('value') == 1

            # returning results in case `fall_through` is True
            handler = make_project_db_handler(True)
            handled_result = handler('placeholder', False, request, bad_result)
            assert handled_result.get('value') == 1

            handled_result = handler('placeholder', True, request, good_result)
            assert handled_result.get('value') == 1

    def test_on_received_data(self):
        manager = Manager()
        with patch.object(manager, 'handle_requestGeneric'):
            hsh = Hash('type', 'requestGeneric', 'value', 'terror')
            manager.onReceivedData(hsh)
            manager.handle_requestGeneric.assert_called_with(value='terror')

    def test_on_server_connection_changed(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                manager.onServerConnectionChanged(True)
                broadcast_event.assert_called_with(
                    KaraboEvent.NetworkConnectStatus,
                    {'status': True}
                )

                # Check that handlers are all called when network disconnects
                target = 'karabogui.singletons.manager.get_logger'

                with patch(target) as logger:
                    manager._request_handlers = {'bob': 'super_request'}
                    manager.onServerConnectionChanged(False)
                    logger().error.assert_called_with(
                        'Erased <b>1</b> pending generic requests due to '
                        'client disconnect.')
                topology.clear.assert_called_with()
                # No further requests
                assert not manager._request_handlers

    def test_handle_device_configuration(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_deviceConfiguration('Pauly', Hash('choochness', 11))
            topology.device_config_updated.assert_called_with(
                'Pauly', Hash('choochness', 11)
            )

    def test_handle_project_list_items(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'items', ['schleem', 'plumbus'])
            manager.handle_projectListItems(True, Hash(), h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsList,
                {'items': ['schleem', 'plumbus']}
            )

    def test_handle_project_list_domains(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'domains', ['WESTEROS'])
            manager.handle_projectListDomains(True, Hash(), h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectDomainsList,
                {'items': ['WESTEROS']}
            )

    def test_handle_project_load_items(self):
        manager = Manager()
        h = Hash('success', False, 'items', ['load_me'], 'reason', 'U SUCK')

        broadcast = 'karabogui.singletons.manager.broadcast_event'
        messagebox = 'karabogui.singletons.manager.messagebox'
        with patch(broadcast) as broadcast_event, patch(messagebox):
            manager.handle_projectLoadItems(True, Hash(), h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsLoaded,
                {'success': False, 'items': ['load_me']}
            )

    def test_handle_project_save_items(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['remember_this'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectSaveItems(True, Hash(), h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsSaved,
                {'items': ['remember_this'], 'success': True}
            )

    def test_handle_project_update_attribute(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['new_stuff'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectUpdateAttribute(True, Hash(), h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectAttributeUpdated,
                {'items': ['new_stuff']}
            )

    def test_handle_projectUpdate(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()
            client = "my_client"
            uuids = ["123", "456"]
            h = Hash('success', True, 'info', {"client": client,
                                               "projects": uuids})
            manager.handle_projectUpdate(**h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectUpdated, {'uuids': ['123', '456']})

    def test_handle_set_log_reply(self):
        manager = Manager()
        target = 'karabogui.messagebox.show_error'
        with patch(target) as mb:
            h = Hash("success", True)
            h["input"] = Hash("instanceId", "swerver",
                              "priority", "DEBUG")
            manager.handle_setLogPriorityReply(**h)
            mb.assert_not_called()
            h = Hash("success", False, "reason", "")
            h["input"] = Hash("instanceId", "swerver",
                              "priority", "DEBUG")
            manager.handle_setLogPriorityReply(**h)
            mb.assert_called_once()

    def test_handle_network_data(self):
        dev_proxy, prop_binding, topology = Mock(), Mock(), Mock()

        executeLater = 'karabogui.singletons.manager.executeLater'
        apply_tgt = 'karabogui.singletons.manager.apply_fast_data'
        with patch(executeLater) as later, patch(apply_tgt) as apply_later:
            with singletons(topology=topology):
                manager = Manager()
                dev_proxy.get_property_binding.return_value = prop_binding
                topology.get_device.return_value = dev_proxy
                ts = Timestamp("2009-04-20T10:32:22 UTC")
                meta = Hash('timestamp', True)
                ts.toHashAttributes(meta)
                data = Hash('Position', 10)
                manager.handle_networkData('frankie:output', data, meta)
                assert 'frankie:output' in manager._big_data
                assert later.call_count == 1

                callback = later.mock_calls[0][1][0]
                callback()
                apply_later.assert_called_with(
                    data, prop_binding.value.schema, ts)

    def test_handle_init_reply(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()

            manager.handle_initReply('rick', True, 'wubbalubbadubdub')
            broadcast_event.assert_called_with(
                KaraboEvent.DeviceInitReply, ANY
            )
            event_data = broadcast_event.mock_calls[0][1][1]
            assert event_data['success']
            assert event_data['message'] == 'wubbalubbadubdub'
            assert isinstance(event_data['device'], DeviceProxy)

            with patch('karabogui.singletons.manager.messagebox') as mbox:
                info = {'deviceId': 'bob', 'message': 'mandatory key missing',
                        'success': False}
                manager = Manager()
                manager.handle_initReply(**info)
                mbox.show_error.assert_called_with(
                    'The instance <b>bob</b> could not be instantiated.. '
                    '<br><br>The reason is:<br><i>mandatory key '
                    'missing</i><br>', details=None)

                # details case
                info = {
                    'deviceId': 'bob',
                    'message': 'mandatory key missing\nDetails:\nhost missing',
                    'success': False}
                manager.handle_initReply(**info)
                mbox.show_error.assert_called_with(
                    'The instance <b>bob</b> could not be instantiated.. '
                    '<br><br>The reason is:<br><i>mandatory key '
                    'missing</i><br><br>Click '
                    '"Show Details..." for more information.',
                    details="host missing")

    def test_handle_log_messages(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()
            messages = [Hash(
                "type", "error",
                "category", "XFEL/MOTOR/1",
                "message", "This is a test message",
                "traceback", "",
                "timestamp", Timestamp().toLocal())]
            manager.handle_log(messages)
            broadcast_event.assert_called_with(
                KaraboEvent.LogMessages, {"messages": messages})

    def test_handle_broker_information(self):
        network = Mock()
        mediator = Mediator()
        with singletons(network=network, mediator=mediator):
            # New protocol
            manager = Manager()
            manager.handle_serverInformation(readOnly=True)
            network.set_server_information.assert_called_once_with(
                read_only=True)
            # Old protocol
            network.reset_mock()
            manager.handle_brokerInformation(readOnly=True)
            network.set_server_information.assert_called_once_with(
                read_only=True)

    def test_handle_login_information(self):
        network = Mock()
        mediator = Mediator()
        path = 'karabogui.singletons.manager.broadcast_event'
        with patch(path) as broad:
            with singletons(network=network, mediator=mediator):
                manager = Manager()
                manager.handle_loginInformation(
                    accessLevel=AccessLevel.ADMIN.value)
                broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
                broad.reset_mock()
                # Try again downgrade
                manager.handle_loginInformation(
                    accessLevel=AccessLevel.OBSERVER.value)
                broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
                broad.reset_mock()
                # Read Only, but since we are observer, it is not chnaged
                manager.handle_loginInformation(
                    accessLevel=AccessLevel.OBSERVER.value)
                broad.assert_not_called()

    def test_handle_alarm_update(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmUpdate('AlarmService', Hash(alarm_data()))

    def test_handle_alarm_init(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmInit('AlarmService', Hash(alarm_data()))

    def test_handle_property_history(self):
        topology, device_proxy = Mock(), Mock()
        with singletons(topology=topology):
            topology.get_device.return_value = device_proxy
            manager = Manager()
            info = {'deviceId': 'bob', 'property': 'answer', 'data': [42],
                    'success': True}
            manager.handle_propertyHistory(**info)
            expected_call = call.publish_historic_data('answer', [42])
            assert device_proxy.method_calls[0] == expected_call

    def test_handle_notification(self):
        m_path = 'karabogui.singletons.manager.messagebox'
        b_path = 'karabogui.singletons.manager.broadcast_event'
        l_path = 'karabogui.singletons.manager.get_logger'

        with patch(m_path) as mbox, patch(b_path) as broadcast:
            info = {'message': 'hello'}
            manager = Manager()
            manager.handle_notification(**info)
            mbox.show_warning.assert_called_with('hello')

            mbox.reset_mock()
            info = {'nomessage': 'hello'}
            manager.handle_notification(**info)
            mbox.show_warning.assert_not_called()

            mbox.reset_mock()
            info = {'message': 'hello', 'contentType': 'banner'}
            manager.handle_notification(**info)
            mbox.show_warning.assert_not_called()
            broadcast.assert_called_with(
                KaraboEvent.ServerNotification, info)

            mbox.reset_mock()
            broadcast.reset_mock()
            with patch(l_path) as logger:
                info = {'message': 'hello', 'contentType': 'logger'}
                manager.handle_notification(**info)
                mbox.show_warning.assert_not_called()
                broadcast.assert_not_called()
                logger().log.assert_called_with(20, 'hello')

                asserts = [
                    ('DEBUG', 10),
                    ('INFO', 20),
                    ('WARNING', 30),
                    ('WARN', 30),
                    ('ERROR', 40),
                    ('FATAL', 50),
                    ('CRITICAL', 50),
                ]
                for level, integer in asserts:
                    logger().reset_mock()
                    info = {'message': 'hello', 'contentType': 'logger',
                            'level': level}
                    manager.handle_notification(**info)
                    logger().log.assert_called_with(integer, 'hello')

    def test_handle_executeReply(self):
        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            info = Hash(
                "success", False,
                "input", Hash("deviceId", "XFEL/MOTOR/2",
                              "command", "move"),
                "reason", "timeout in gui server\nDetails:\nNot online")
            manager.handle_executeReply(**info)
            mbox.show_error.assert_called_with(
                'Execute slot <b>move</b> of device <b>XFEL/MOTOR/2</b> has '
                'encountered an error!<br><br>The reason is:'
                '<br><i>timeout in gui server</i><br><br>Click '
                '"Show Details..." for more information.',
                details="Not online")

            # No details test
            info = Hash(
                "success", False,
                "input", Hash("deviceId", "XFEL/MOTOR/2",
                              "command", "move"),
                "reason", "timeout in gui server")
            manager.handle_executeReply(**info)
            mbox.show_error.assert_called_with(
                'Execute slot <b>move</b> of device <b>XFEL/MOTOR/2</b> has '
                'encountered an error!<br><br>The reason is:'
                '<br><i>timeout in gui server</i><br>',
                details=None)

    def test_handle_deviceConfigurations(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            configs = {"XFEL/MOTOR/1": Hash("state", "MOVING"),
                       "XFEL/MOTOR/2": Hash("state", "OFF")}
            manager.handle_deviceConfigurations(configs)
            topology.device_config_updated.call_count == 2

    def test_handle_configurationFromPast(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()
            config_time = Timestamp()
            config = Hash("archive", False)
            info = Hash(
                "success", True,
                "config", config,
                "reason", "",
                "deviceId", "XFEL/MOTOR/2",
                "configTimepoint", config_time,
                "configAtTimepoint", True,
                "preview", True,
                "time", config_time)

            manager.handle_configurationFromPast(**info)
            broadcast_event.assert_called_with(
                KaraboEvent.ShowConfigurationFromPast,
                {'deviceId': 'XFEL/MOTOR/2',
                 'configuration': config,
                 'time': config_time.toLocal(),
                 'config_time': config_time.toLocal(),
                 'preview': True, 'time_match': True})

        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            timepoint = Timestamp()
            info = Hash(
                "success", False,
                "deviceId", "XFEL/MOTOR/2",
                "time", timepoint)
            reason = "Device not logged"
            info["reason"] = reason
            manager.handle_configurationFromPast(**info)
            mbox.show_error.assert_called_with(
                "The configuration of `XFEL/MOTOR/2` requested at time point "
                f"`{timepoint.toLocal()}` was not retrieved!<br><br>"
                f"The reason is:<br><i>{reason}</i>",
                details=None)

    def test_handle_listConfigurationFromName(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()
            items = ["a", "b", "c"]
            args = Hash("deviceId", "XFEL/CAM/1")
            info = Hash(
                "success", True,
                "reply", Hash("items", items),
                "request", Hash("args", args),
                "reason", "")

            manager.handle_listConfigurationFromName(**info)
            broadcast_event.assert_called_with(
                KaraboEvent.ListConfigurationUpdated,
                {'items': items, 'deviceId': 'XFEL/CAM/1'})

        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            info["success"] = False
            manager.handle_listConfigurationFromName(**info)
            mbox.show_error.assert_called_with(
                'Requesting a list of configurations for XFEL/CAM/1 failed!',
                details='')

    def test_handle_getConfigurationFromName(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()
            item = Hash("name", "devName",
                        "config", Hash("state", "ON"))

            args = Hash("deviceId", "XFEL/CAM/1")
            info = Hash(
                "success", True,
                "reply", Hash("item", item),
                "request", Hash("args", args, "preview", False),
                "reason", "")

            manager.handle_getConfigurationFromName(**info)
            broadcast_event.assert_called_with(
                KaraboEvent.ShowConfigurationFromName,
                {'configuration': Hash("state", "ON"), 'preview': False,
                 'name': 'devName', 'deviceId': 'XFEL/CAM/1'})

        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            info["success"] = False
            manager.handle_getConfigurationFromName(**info)
            mbox.show_error.assert_called_with(
                'Requesting a configuration for XFEL/CAM/1 failed!',
                details='')

    def test_handle_saveConfigurationFromName(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            item = Hash("name", "devName",
                        "config", Hash("state", "ON"))

            args = Hash("deviceIds", ["XFEL/CAM/1"])
            info = Hash(
                "success", True,
                "reply", Hash("item", item),
                "request", Hash("args", args, "preview", False,
                                "update", True),
                "reason", "")
            manager.handle_saveConfigurationFromName(**info)
            network.onListConfigurationFromName.assert_called_with(
                'XFEL/CAM/1')

        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            info["success"] = False
            manager.handle_saveConfigurationFromName(**info)
            mbox.show_error.assert_called_with(
                'Saving a configuration for XFEL/CAM/1 failed!', details='')

    def test_handle_subscribeLogsReply(self):
        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as mbox:
            manager = Manager()
            info = {"success": False}
            manager.handle_subscribeLogsReply(**info)
            mbox.show_error.assert_called_with(
                'Could not reconfigure the logs for the gui server')
