from unittest.mock import ANY, Mock, call, patch

from PyQt4.QtGui import QMessageBox

from karabo.common.api import DeviceStatus
from karabo.middlelayer import AccessMode, Configurable, Hash, Int32, Schema
from karabogui.binding.api import build_binding, DeviceClassProxy, DeviceProxy
from karabogui.events import KaraboEventSender
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
                mb.question.return_value = QMessageBox.Yes
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_called_with('dev')
                network.reset_mock()
                mb.reset_mock()
                manager.shutdownDevice('dev', showConfirm=False)
                mb.question.return_value = QMessageBox.No
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_not_called()

            with patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb.question.return_value = QMessageBox.Yes
                network.reset_mock()
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_called_with('dev')

                mb.question.return_value = QMessageBox.No
                network.reset_mock()
                manager.shutdownDevice('dev')
                network.onKillDevice.assert_not_called()

    def test_shutdown_server(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            with patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb.question.return_value = QMessageBox.Yes
                manager.shutdownServer('swerver')
                network.onKillServer.assert_called_with('swerver')
                mb.reset_mock()
                mb.question.return_value = QMessageBox.No
                manager.shutdownServer('swerver')
                network.onKillServer.assert_not_called()

                mb.question.return_value = QMessageBox.No
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
                'token', 'dev', 'slot', params)

            # Pretend the reply arrived
            manager.handle_requestFromSlot('token', True, reply=reply)
            assert handler_args == (True, reply)

    def test_handle_system_topology(self):
        topo_hash = system_hash()
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_systemTopology(topo_hash)
            topology.update.assert_called_with(topo_hash)

    def test_handle_instance_new(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()

            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                topo_hash = system_hash()
                del topo_hash['device.divvy']
                del topo_hash['macro']

                manager.handle_instanceNew(topo_hash)

                topo_update = {
                    'devices': [('orphan', 'Parentless',
                                 DeviceStatus.NOSERVER)],
                    'servers': [('swerver', 'BIG_IRON', DeviceStatus.OK)]
                }
                broadcast_event.assert_called_with(
                    KaraboEventSender.SystemTopologyUpdate, topo_update)

    def test_handle_instance_gone(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()

            devices = [('orphan', 'Parentless', DeviceStatus.OFFLINE)]
            topology.instance_gone.return_value = (devices, [])

            calls = [
                call(KaraboEventSender.SystemTopologyUpdate,
                     {'devices': devices, 'servers': []}),
                call(KaraboEventSender.ClearConfigurator,
                     {'deviceId': 'orphan'})
            ]

            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                manager.handle_instanceGone('orphan', 'device')
                broadcast_event.assert_has_calls(calls)

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
            msg_box.show_error.assert_called_with('error_msg', modal=False)

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
        with patch.object(manager, 'handle_requestFromSlot'):
            hsh = Hash('type', 'requestFromSlot', 'value', 'terror')
            manager.onReceivedData(hsh)
            manager.handle_requestFromSlot.assert_called_with(value='terror')

    def test_on_server_connection_changed(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            target = 'karabogui.singletons.manager.broadcast_event'
            with patch(target) as broadcast_event:
                manager.onServerConnectionChanged(True)
                broadcast_event.assert_called_with(
                    KaraboEventSender.NetworkConnectStatus,
                    {'status': True}
                )

            # Check that handlers are all called when network disconnects
            with patch.object(manager, 'handle_requestFromSlot'):
                manager._request_handlers = {'bob': 'super_request'}
                manager.onServerConnectionChanged(False)
                manager.handle_requestFromSlot.assert_called_with(
                    'bob', False, info=None
                )
            topology.clear.assert_called_with()

    def test_handle_device_configuration(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_deviceConfiguration('Pauly', Hash('choochness', 11))
            topology.device_config_updated.assert_called_with(
                'Pauly', Hash('choochness', 11)
            )

    def test_handle_instance_updated(self):
        topology = Mock()
        with singletons(topology=topology):
            manager = Manager()
            manager.handle_instanceUpdated(Hash())
            topology.instance_updated.assert_called_with(Hash())

    def test_handle_project_list_items(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'items', ['schleem', 'plumbus'])
            manager.handle_projectListItems(h)
            broadcast_event.assert_called_with(
                KaraboEventSender.ProjectItemsList,
                {'items': ['schleem', 'plumbus']}
            )

    def test_handle_project_list_domains(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'domains', ['WESTEROS'])
            manager.handle_projectListDomains(h)
            broadcast_event.assert_called_with(
                KaraboEventSender.ProjectDomainsList,
                {'items': ['WESTEROS']}
            )

    def test_handle_project_list_project_managers(self):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            h = Hash('success', True, 'items', ['fast_eddy'])
            manager.handle_projectListProjectManagers(h)
            broadcast_event.assert_called_with(
                KaraboEventSender.ProjectManagersList,
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
                KaraboEventSender.ProjectItemsLoaded,
                {'success': False, 'items': ['load_me']}
            )

    def test_handle_project_save_items(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['remember_this'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectSaveItems(h)
            broadcast_event.assert_called_with(
                KaraboEventSender.ProjectItemsSaved,
                {'items': ['remember_this']}
            )

    def test_handle_project_update_attribute(self):
        manager = Manager()
        h = Hash('success', True, 'items', ['new_stuff'])

        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager.handle_projectUpdateAttribute(h)
            broadcast_event.assert_called_with(
                KaraboEventSender.ProjectAttributeUpdated,
                {'items': ['new_stuff']}
            )

    def test_handle_network_data(self):
        dev_proxy, prop_binding, topology = Mock(), Mock(), Mock()

        executeLater = 'karabogui.singletons.manager.executeLater'
        apply_tgt = 'karabogui.singletons.manager.apply_configuration'
        with patch(executeLater) as later, patch(apply_tgt) as apply_later:
            with singletons(topology=topology):
                manager = Manager()
                dev_proxy.get_property_binding.return_value = prop_binding
                topology.get_device.return_value = dev_proxy

                manager.handle_networkData('frankie:a', Hash('a', 10))
                assert 'frankie:a' in manager._big_data
                assert later.call_count == 1

                callback = later.mock_calls[0][1][0]
                callback()
                apply_later.assert_called_with(Hash('a', 10),
                                               prop_binding.value.schema)

    def test_handle_init_reply(self):
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast_event:
            manager = Manager()

            manager.handle_initReply('rick', True, 'wubbalubbadubdub')
            broadcast_event.assert_called_with(
                KaraboEventSender.DeviceInitReply, ANY
            )
            event_data = broadcast_event.mock_calls[0][1][1]
            assert event_data['success']
            assert event_data['message'] == 'wubbalubbadubdub'
            assert isinstance(event_data['device'], DeviceProxy)

    def test_handle_alarm_update(self):
        topology = Mock()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast, singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmUpdate('AlarmService', Hash(alarm_data()))
            assert topology.update_alarms_info.call_count == 1
            assert broadcast.call_count == 1

    def test_handle_alarm_init(self):
        topology = Mock()
        target = 'karabogui.singletons.manager.broadcast_event'
        with patch(target) as broadcast, singletons(topology=topology):
            manager = Manager()
            manager.handle_alarmInit('AlarmService', Hash(alarm_data()))
            assert topology.update_alarms_info.call_count == 1
            assert broadcast.call_count == 1

    def test_handle_broker_information(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()
            manager.handle_brokerInformation(one=2, two=3, five=4)
            network._handleBrokerInformation.assert_called_once_with(
                one=2, two=3, five=4)

    def test_handle_property_history(self):
        topology, device_proxy = Mock(), Mock()
        with singletons(topology=topology):
            topology.get_device.return_value = device_proxy
            manager = Manager()

            manager.handle_propertyHistory('bob', 'answer', [42])
            expected_call = call.publish_historic_data('answer', [42])
            assert device_proxy.method_calls[0] == expected_call
