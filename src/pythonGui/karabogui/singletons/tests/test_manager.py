from unittest.mock import Mock, call, patch

from PyQt4.QtGui import QMessageBox

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Hash, Schema
from karabogui.events import KaraboEventSender
from karabogui.testing import GuiTestCase, singletons, system_hash
from ..manager import Manager


class TestManager(GuiTestCase):
    def test_shutdown_device(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            manager.shutdownDevice('dev', showConfirm=False)
            network.onKillDevice.assert_called_with('dev')

    def test_shutdown_server(self):
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            with patch('karabogui.singletons.manager.QMessageBox') as mbklass:
                mbklass.question.return_value = QMessageBox.Yes
                manager.shutdownServer('swerver')
                network.onKillServer.assert_called_with('swerver')

    def test_call_device_slot(self):
        handler_args = None

        def handler(*args):
            nonlocal handler_args
            handler_args = args

        params = Hash('arg0', 0, 'arg1', 1)
        reply = Hash('result', 'yer dumb')

        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
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
        network, topology = Mock(), Mock()
        with singletons(network=network, topology=topology):
            manager = Manager()
            manager.handle_systemTopology(topo_hash)
            topology.update.assert_called_with(topo_hash)

    def test_handle_instance_new(self):
        network = Mock()
        with singletons(network=network):
            manager = Manager()

            with patch('karabogui.singletons.manager.broadcast_event') as b_e:
                topo_hash = system_hash()
                del topo_hash['device.divvy']
                del topo_hash['macro']

                manager.handle_instanceNew(topo_hash)

                topo_update = {
                    'devices': [('orphan', 'Parentless',
                                 DeviceStatus.NOSERVER)],
                    'servers': [('swerver', 'BIG_IRON', DeviceStatus.OK)]
                }
                b_e.assert_called_with(
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

            with patch('karabogui.singletons.manager.broadcast_event') as b_e:
                manager.handle_instanceGone('orphan', 'device')
                b_e.assert_has_calls(calls)

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
