from unittest.mock import ANY, Mock, call, patch
from qtpy.QtCore import QSize

from karabo.common.api import ProxyStatus
from karabo.native import (
    AccessMode, Configurable, Hash, Int32, Schema, Timestamp)
from karabogui.binding.api import build_binding, DeviceClassProxy, DeviceProxy
from karabogui.events import KaraboEvent
from karabogui.testing import alarm_data, GuiTestCase, singletons, system_hash
from ..manager import Manager, project_db_handler


def make_project_db_handler(fall_through):
    # Creates a wrapped function to test the project_db_handler decorator
    @project_db_handler(fall_through)
    def handle(self, reply):
        return reply

    return handle


class PrettyDevice(Configurable):
    init_prop = Int32(accessMode=AccessMode.INITONLY)
    ro_prop = Int32(accessMode=AccessMode.READONLY)


def _get_class_proxy(schema):
    binding = build_binding(schema)
    return DeviceClassProxy(binding=binding)


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

        def handler(*args):
            nonlocal handler_args
            handler_args = args

        params = Hash('arg0', 0, 'arg1', 1)
        reply = Hash('result', 'yer dumb')

        network = Mock()
        with singletons(network=network):
            manager = Manager()

            # Call the slot of the 'remote device'
            manager.callDeviceSlot('token', handler, 'dev', 'slot', params)

            assert 'token' in manager._request_handlers
            network.onExecuteGeneric.assert_called_with(
                'dev', 'slot', params)

            # Pretend the reply arrived
            manager.handle_requestGeneric(success=True,
                                          request=Hash("args.token", 'token'),
                                          reply=reply)
            assert handler_args == (True, reply)

    def test_handle_system_topology(self):
        topo_hash = system_hash()
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_systemTopology(topo_hash)
            topology.initialize.assert_called_with(topo_hash)

    def test_handle_instance_new(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()

            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                topo_hash = system_hash()
                manager._topology.initialize(topo_hash)

                topo_hash = system_hash()
                del topo_hash['device.divvy']
                del topo_hash['macro']

                changes = Hash("new", topo_hash,
                               "update", Hash(), "gone", Hash())
                manager.handle_topologyUpdate(changes)

                topo_update = {
                    'devices': [('orphan', 'Parentless',
                                 ProxyStatus.NOSERVER)],
                    'servers': [('swerver', 'BIG_IRON', ProxyStatus.OK)]
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
        with singletons(topology=topology):
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
        with singletons(network=network, topology=topology):
            manager = Manager()

            schema = Schema()
            reply = Hash('instanceId', 'dev', 'updatedSchema', schema)
            manager.handle_attributesUpdated(reply)
            topology.device_schema_updated.assert_called_with('dev', schema)

            manager.handle_deviceSchema('dev', schema)
            topology.device_schema_updated.assert_called_with('dev', schema)

    def test_init_device_empty_class(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            # instantiate with wrong class def
            manager = Manager()
            empty_schema = Schema('PrettyDevice', hash=Hash())
            dkp = _get_class_proxy(empty_schema)
            topology.get_schema.return_value = empty_schema
            topology.get_class.return_value = dkp
            topology.get_project_device_proxy.return_value = None

            cfg = Hash('init_prop', 42, 'ro_prop', -1)
            manager.initDevice('swerver', 'PrettyDevice', 'dev', config=cfg)
            topology.get_schema.assert_called_with('swerver', 'PrettyDevice')
            topology.get_class.assert_called_with('swerver', 'PrettyDevice')
            network.onInitDevice.assert_called_with(
                'swerver', 'PrettyDevice', 'dev',
                # The configuration hash will be stripped of all keys due to
                # the PrettyDevice class having an empty schema.
                Hash(), attrUpdates=None
            )

    def test_init_device_schema_evolution(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            schema = PrettyDevice.getClassSchema()
            dkp = _get_class_proxy(schema)
            topology.get_schema.return_value = schema
            topology.get_class.return_value = dkp
            topology.get_project_device_proxy.return_value = None

            cfg = Hash('init_prop', 42, 'ro_prop', -1, 'evolved', 43)
            manager.initDevice('swerver', 'PrettyDevice', 'dev', config=cfg)
            topology.get_schema.assert_called_with('swerver', 'PrettyDevice')
            topology.get_class.assert_called_with('swerver', 'PrettyDevice')
            network.onInitDevice.assert_called_with(
                'swerver', 'PrettyDevice', 'dev',
                # The configuration will be stripped of all the keys
                # that are read-only or not in the schema
                Hash('init_prop', 42), attrUpdates=None
            )

    def test_init_device_empty_config(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            schema = PrettyDevice.getClassSchema()
            dkp = _get_class_proxy(schema)
            topology.get_schema.return_value = schema
            topology.get_class.return_value = dkp
            topology.get_project_device_proxy.return_value = None

            manager.initDevice('swerver', 'PrettyDevice', 'dev')
            topology.get_schema.assert_called_with('swerver', 'PrettyDevice')
            topology.get_class.assert_called_with('swerver', 'PrettyDevice')
            network.onInitDevice.assert_called_with(
                'swerver', 'PrettyDevice', 'dev', Hash(), attrUpdates=None)

    def test_init_device_badschema(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            cfg = Hash('init_prop', 42, 'ro_prop', -1)
            topology.get_schema.return_value = None

            target = 'karabogui.singletons.manager.messagebox'
            with patch(target) as msg_box:
                cfg = Hash('init_prop', 42, 'ro_prop', -1)
                manager.initDevice('swerver', 'PrettyDevice', 'dev',
                                   config=cfg)
                assert msg_box.show_warning.call_count == 1

    def test_project_db_handler(self):
        bad_result = Hash('success', False, 'value', 1, 'reason', 'error_msg')
        good_result = Hash('success', True, 'value', 1)
        target = 'karabogui.singletons.manager.messagebox'
        with patch(target) as msg_box:
            # this creates a function wrapped by the handler that returns
            # the input data
            handler = make_project_db_handler(False)
            handler('placeholder', bad_result)
            # error shown in case of bad result
            msg_box.show_error.assert_called_with('error_msg')

            handled_result = handler('placeholder', good_result)
            assert handled_result.get('value') == 1

            # returning results in case `fall_through` is True
            handler = make_project_db_handler(True)
            handled_result = handler('placeholder', bad_result)
            assert handled_result.get('value') == 1

            handled_result = handler('placeholder', good_result)
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
            with patch.object(manager, 'handle_requestGeneric'):
                manager._request_handlers = {'bob': 'super_request'}
                manager.onServerConnectionChanged(False)
                manager.handle_requestGeneric.assert_called_with(
                    False, request=Hash('args.token', 'bob'),
                    reason='Karabo GUI Client disconnect. Erasing request.')
            topology.clear.assert_called_with()

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
            manager.handle_projectListItems(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsList,
                {'items': ['schleem', 'plumbus']}
            )

    def test_handle_project_list_domains(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'domains', ['WESTEROS'])
            manager.handle_projectListDomains(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectDomainsList,
                {'items': ['WESTEROS']}
            )

    def test_handle_project_list_project_managers(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'items', ['fast_eddy'])
            manager.handle_projectListProjectManagers(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectManagersList,
                {'items': h}
            )

    def test_handle_project_load_items(self):
        manager = Manager()
        h = Hash('success', False, 'items', ['load_me'], 'reason', 'U SUCK')

        broadcast = 'karabogui.singletons.manager.broadcast_event'
        messagebox = 'karabogui.singletons.manager.messagebox'
        with patch(broadcast) as broadcast_event, patch(messagebox):
            manager.handle_projectLoadItems(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsLoaded,
                {'success': False, 'items': ['load_me']}
            )

    def test_handle_project_save_items(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['remember_this'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectSaveItems(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectItemsSaved,
                {'items': ['remember_this'], 'success': True}
            )

    def test_handle_project_update_attribute(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['new_stuff'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectUpdateAttribute(h)
            broadcast_event.assert_called_with(
                KaraboEvent.ProjectAttributeUpdated,
                {'items': ['new_stuff']}
            )

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

    def test_handle_alarm_update(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmUpdate('AlarmService', Hash(alarm_data()))
            assert topology.update_alarms_info.call_count == 1

    def test_handle_alarm_init(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmInit('AlarmService', Hash(alarm_data()))
            assert topology.update_alarms_info.call_count == 1

    def test_handle_broker_information(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            manager.handle_brokerInformation(readOnly=True)
            network.set_server_information.assert_called_once_with(
                read_only=True)

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
