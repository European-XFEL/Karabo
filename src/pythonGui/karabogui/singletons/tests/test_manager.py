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
from inspect import signature

from qtpy.QtCore import QSize

from karabo.common.api import WeakMethodRef
from karabo.native import (
    AccessLevel, AccessMode, Configurable, Hash, Int32, Schema, Timestamp)
from karabogui import access as krb_access
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, ProjectDeviceProxy,
    ProxyStatus, build_binding)
from karabogui.events import KaraboEvent
from karabogui.testing import singletons, system_hash
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


def test_shutdown_device(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        manager.shutdownDevice('dev', showConfirm=False)
        network.onKillDevice.assert_called_with('dev')
        network.reset_mock()
        manager.shutdownDevice('dev', showConfirm=False)
        mb = mocker.patch('karabogui.singletons.manager.QMessageBox')
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

        mb = mocker.patch('karabogui.singletons.manager.QMessageBox')
        mb().size.return_value = QSize(10, 10)
        mb().exec.return_value = mb.Yes
        network.reset_mock()
        manager.shutdownDevice('dev')
        network.onKillDevice.assert_called_with('dev')

        mb().exec.return_value = mb.No
        network.reset_mock()
        manager.shutdownDevice('dev')
        network.onKillDevice.assert_not_called()


def test_shutdown_server(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        mb = mocker.patch('karabogui.singletons.manager.QMessageBox')
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


def test_call_device_slot(mocker):
    handler_args = None

    def handler(success, reply):
        nonlocal handler_args
        handler_args = success, reply

    def request_handler(success, reply, request):
        nonlocal handler_args
        handler_args = success, reply, request

    params = Hash('arg0', 0, 'arg1', 1)
    reply = Hash('result', 'yer dumb')

    network = mocker.Mock()
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

        class TestObject:
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


def test_handle_system_topology(gui_app, mocker):
    topo_hash = system_hash()
    topology = mocker.Mock()
    mediator = Mediator()
    with singletons(topology=topology, mediator=mediator):
        manager = Manager()
        manager.handle_systemTopology(topo_hash)
        topology.initialize.assert_called_with(topo_hash)


def test_handle_instance_new(mocker):
    network = mocker.Mock()
    topology = SystemTopology()
    topo_hash = system_hash()
    topology.initialize(topo_hash)
    with singletons(network=network, topology=topology):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        broadcast_event = mocker.patch(target)
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


def test_handle_instance_gone(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
    with singletons(network=network, topology=topology):
        manager = Manager()

        devices = [('orphan', 'Parentless', ProxyStatus.OFFLINE)]
        topology.topology_update.return_value = (devices, [])

        calls = [
            mocker.call(KaraboEvent.SystemTopologyUpdate,
                        {'devices': devices, 'servers': []}),
            mocker.call(KaraboEvent.ClearConfigurator,
                        {'devices': ['orphan']})
        ]

        target = 'karabogui.singletons.manager.broadcast_event'
        broadcast_event = mocker.patch(target)
        changes = Hash("new", Hash(),
                       "update", Hash(),
                       "gone", Hash("device", "orphan"))
        manager.handle_topologyUpdate(changes)
        broadcast_event.assert_has_calls(calls)


def test_handle_instance_updated(gui_app, mocker):
    topology = mocker.Mock()
    mediator = Mediator()
    with singletons(topology=topology, mediator=mediator):
        topology.topology_update.return_value = [], []
        manager = Manager()
        changes = Hash("new", Hash(), "update", Hash(),
                       "gone", Hash())
        manager.handle_topologyUpdate(changes)
        topology.topology_update.assert_called_with(changes)


def test_handle_class_schema(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
    with singletons(network=network, topology=topology):
        manager = Manager()

        schema = Schema()
        manager.handle_classSchema('server', 'ClassName', schema)

        topology.class_schema_updated.assert_called_with(
            'server', 'ClassName', schema)


def test_init_device_none(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
    with singletons(network=network, topology=topology):
        # instantiate with wrong class def
        manager = Manager()
        topology.get_project_device_proxy.return_value = None

        cfg = Hash('init_prop', 42, 'ro_prop', -1)
        manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID, TEST_DEVICE_ID,
                           config=cfg)
        # No proxy means, not called!
        network.onInitDevice.assert_not_called()


def test_init_device_schema_evolution(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
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
            Hash('init_prop', 42))


def test_init_device_empty_config(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
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
            Hash())


def test_init_device_badschema(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
    with singletons(network=network, topology=topology):
        manager = Manager()
        topology.get_schema.return_value = None
        proxy = _get_project_device_proxy(TEST_SERVER_ID, TEST_CLASS_ID,
                                          TEST_DEVICE_ID)
        topology.get_project_device_proxy.return_value = proxy

        target = 'karabogui.messagebox.show_error'
        msg_box = mocker.patch(target)
        cfg = Hash('init_prop', 42, 'ro_prop', -1)
        manager.initDevice(TEST_SERVER_ID, TEST_CLASS_ID,
                           TEST_DEVICE_ID, config=cfg)
        assert msg_box.call_count == 1


def test_project_db_handler(mocker):
    bad_result = Hash('value', 1, 'reason', 'error_msg')
    good_result = Hash('value', 1)
    target = 'karabogui.singletons.manager.messagebox'
    msg_box = mocker.patch(target)
    # this creates a function wrapped by the handler that returns
    # the input data
    request = Hash()
    handler = make_project_db_handler(False)
    handler('placeholder', False, request, bad_result, reason="error_msg")
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


def test_on_received_data(gui_app, mocker):
    manager = Manager()
    mocker.patch.object(manager, 'handle_requestGeneric')
    hsh = Hash('type', 'requestGeneric', 'value', 'terror')
    manager.onReceivedData(hsh)
    manager.handle_requestGeneric.assert_called_with(value='terror')


def test_on_server_connection_changed(mocker):
    network, topology = mocker.Mock(), mocker.Mock()
    with singletons(network=network, topology=topology):
        manager = Manager()
        target = 'karabogui.singletons.manager.broadcast_event'
        broadcast_event = mocker.patch(target)
        manager.onServerConnectionChanged(True)
        broadcast_event.assert_called_with(
            KaraboEvent.NetworkConnectStatus,
            {'status': True}
        )

        # Check that handlers are all called when network disconnects
        target = 'karabogui.singletons.manager.get_logger'

        logger = mocker.patch(target)
        manager._request_handlers = {'bob': 'super_request'}
        manager.onServerConnectionChanged(False)
        logger().error.assert_called_with(
            'Erased <b>1</b> pending generic requests due to '
            'client disconnect.')
        topology.clear.assert_called_with()
        # No further requests
        assert not manager._request_handlers


def test_handle_device_configuration(gui_app, mocker):
    topology = mocker.Mock()
    with singletons(topology=topology):
        manager = Manager()
        manager.handle_deviceConfiguration('Pauly', Hash('choochness', 11))
        topology.device_config_updated.assert_called_with(
            'Pauly', Hash('choochness', 11)
        )


def test_handle_project_list_items(gui_app, mocker):
    manager = Manager()
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    h = Hash('success', True, 'items', ['schleem', 'plumbus'])
    manager.handle_projectListItems(True, Hash(), h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectItemsList,
        {'items': ['schleem', 'plumbus']}
    )


def test_handle_project_list_domains(gui_app, mocker):
    manager = Manager()
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    h = Hash('success', True, 'domains', ['WESTEROS'])
    manager.handle_projectListDomains(True, Hash(), h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectDomainsList,
        {'items': ['WESTEROS']}
    )


def test_handle_project_load_items(gui_app, mocker):
    manager = Manager()
    h = Hash('items', ['load_me'], 'reason', 'we failed')

    broadcast = 'karabogui.singletons.manager.broadcast_event'
    messagebox = 'karabogui.singletons.manager.messagebox'
    broadcast_event = mocker.patch(broadcast)
    mocker.patch(messagebox)
    manager.handle_projectLoadItems(False, Hash(), h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectItemsLoaded,
        {'success': False, 'items': ['load_me']}
    )


def test_handle_project_save_items(gui_app, mocker):
    manager = Manager()
    h = Hash('success', True, 'items', ['remember_this'])

    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager.handle_projectSaveItems(True, Hash(), h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectItemsSaved,
        {'items': ['remember_this'], 'success': True}
    )


def test_handle_project_update_trashed(gui_app, mocker):
    manager = Manager()
    h = Hash('domain', "CONTROLS")

    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager.handle_projectUpdateTrashed(True, Hash(), h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectTrashed,
        {'domain': 'CONTROLS'}
    )


def test_handle_projectUpdate(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager = Manager()
    client = "my_client"
    uuids = ["123", "456"]
    h = Hash('success', True, 'info', {"client": client,
                                       "projects": uuids})
    manager.handle_projectUpdate(**h)
    broadcast_event.assert_called_with(
        KaraboEvent.ProjectUpdated, {'uuids': ['123', '456']})


def test_handle_set_log_reply(gui_app, mocker):
    manager = Manager()
    target = 'karabogui.messagebox.show_error'
    mb = mocker.patch(target)
    h = Hash("success", True)
    h["input"] = Hash("instanceId", "swerver",
                      "level", "DEBUG")
    manager.handle_setLogLevelReply(**h)
    mb.assert_not_called()
    h = Hash("success", False, "reason", "")
    h["input"] = Hash("instanceId", "swerver",
                      "level", "DEBUG")
    manager.handle_setLogLevelReply(**h)
    mb.assert_called_once()


def test_handle_network_data(gui_app, mocker):
    dev_proxy = mocker.Mock()
    prop_binding = mocker.Mock()
    topology = mocker.Mock()

    executeLater = 'karabogui.singletons.manager.executeLater'
    apply_tgt = 'karabogui.singletons.manager.apply_fast_data'
    later = mocker.patch(executeLater)
    apply_later = mocker.patch(apply_tgt)
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
        apply_later.assert_called_with(data, prop_binding.value.schema, ts)


def test_handle_init_reply(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager = Manager()

    manager.handle_initReply('rick', True, 'wubbalubbadubdub')
    broadcast_event.assert_called_with(
        KaraboEvent.DeviceInitReply, mocker.ANY
    )
    event_data = broadcast_event.mock_calls[0][1][1]
    assert event_data['success']
    assert event_data['message'] == 'wubbalubbadubdub'
    assert isinstance(event_data['device'], DeviceProxy)

    mbox = mocker.patch('karabogui.singletons.manager.messagebox')
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


def test_handle_log_messages(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
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


def test_handle_broker_information(mocker):
    network = mocker.Mock()
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


def test_handle_login_information(mocker):
    network = mocker.Mock()
    mediator = Mediator()
    path = 'karabogui.singletons.manager.broadcast_event'
    broad = mocker.patch(path)
    with singletons(network=network, mediator=mediator):
        manager = Manager()
        manager.handle_loginInformation(
            accessLevel=AccessLevel.EXPERT.value, username="karabo")
        broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
        broad.reset_mock()
        # Try again downgrade
        manager.handle_loginInformation(
            accessLevel=AccessLevel.OBSERVER.value, username="karabo")
        broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
        broad.reset_mock()
        # Read Only, but since we are observer, it is not changed
        manager.handle_loginInformation(
            accessLevel=AccessLevel.OBSERVER.value, username="karabo")
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel.OBSERVER
        broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
        broad.reset_mock()

        manager.handle_loginInformation(
            readOnly=True, username="karabo")
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel.OBSERVER
        broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
        broad.reset_mock()

        manager.handle_loginInformation(
            accessLevel=AccessLevel.EXPERT.value, username="karabo")
        broad.assert_called_with(KaraboEvent.LoginUserChanged, {})
        broad.reset_mock()
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel.EXPERT


def test_handle_property_history(gui_app, mocker):
    topology, device_proxy = mocker.Mock(), mocker.Mock()
    with singletons(topology=topology):
        topology.get_device.return_value = device_proxy
        manager = Manager()
        info = {'deviceId': 'bob', 'property': 'answer', 'data': [42],
                'success': True}
        manager.handle_propertyHistory(**info)
        expected_call = mocker.call.publish_historic_data('answer', [42])
        assert device_proxy.method_calls[0] == expected_call


def test_handle_notification(gui_app, mocker):
    m_path = 'karabogui.singletons.manager.messagebox'
    b_path = 'karabogui.singletons.manager.broadcast_event'
    l_path = 'karabogui.singletons.manager.get_logger'

    mbox = mocker.patch(m_path)
    broadcast = mocker.patch(b_path)
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
    logger = mocker.patch(l_path)
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
        info = {'message': 'hello', 'contentType': 'logger', 'level': level}
        manager.handle_notification(**info)
        logger().log.assert_called_with(integer, 'hello')


def test_handle_executeReply(gui_app, mocker):
    target = 'karabogui.singletons.manager.messagebox'
    mbox = mocker.patch(target)
    manager = Manager()
    info = Hash(
        "success", False,
        "input", Hash("deviceId", "XFEL/MOTOR/2", "command", "move"),
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
        "input", Hash("deviceId", "XFEL/MOTOR/2", "command", "move"),
        "reason", "timeout in gui server")
    manager.handle_executeReply(**info)
    mbox.show_error.assert_called_with(
        'Execute slot <b>move</b> of device <b>XFEL/MOTOR/2</b> has '
        'encountered an error!<br><br>The reason is:'
        '<br><i>timeout in gui server</i><br>',
        details=None)


def test_handle_deviceConfigurations(gui_app, mocker):
    topology = mocker.Mock()
    with singletons(topology=topology):
        manager = Manager()
        configs = {"XFEL/MOTOR/1": Hash("state", "MOVING"),
                   "XFEL/MOTOR/2": Hash("state", "OFF")}
        manager.handle_deviceConfigurations(configs)
        topology.device_config_updated.call_count == 2


def test_handle_configurationFromPast(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
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
    mbox = mocker.patch(target)
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


def test_handle_listInitConfigurations(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager = Manager()
    items = ["a", "b", "c"]
    args = Hash("deviceId", "XFEL/CAM/1")
    info = Hash(
        "success", True,
        "reply", Hash("items", items),
        "request", Hash("args", args),
        "reason", "")

    manager.handle_listInitConfigurations(**info)
    broadcast_event.assert_called_with(
        KaraboEvent.ListConfigurationUpdated,
        {'items': items, 'deviceId': 'XFEL/CAM/1'})

    target = 'karabogui.singletons.manager.messagebox'
    mbox = mocker.patch(target)
    manager = Manager()
    info["success"] = False
    manager.handle_listInitConfigurations(**info)
    mbox.show_error.assert_called_with(
        'Requesting a list of configurations for XFEL/CAM/1 failed!',
        details='')


def test_handle_getInitConfiguration(gui_app, mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager = Manager()
    item = Hash("name", "devName",
                "config", Hash("state", "ON"),
                "classId", "FooClass")

    args = Hash("deviceId", "XFEL/CAM/1")
    info = Hash(
        "success", True,
        "reply", Hash("item", item),
        "request", Hash("args", args, "preview", False),
        "reason", "")

    manager.handle_getInitConfiguration(**info)
    broadcast_event.assert_called_with(
        KaraboEvent.ShowInitConfiguration,
        {'configuration': Hash("state", "ON"), 'preview': False,
         'classId': 'FooClass',
         'name': 'devName', 'deviceId': 'XFEL/CAM/1'})

    target = 'karabogui.singletons.manager.messagebox'
    mbox = mocker.patch(target)
    manager = Manager()
    info["success"] = False
    manager.handle_getInitConfiguration(**info)
    mbox.show_error.assert_called_with(
        'Requesting a configuration for XFEL/CAM/1 failed!', details='')


def test_handle_saveInitConfiguration(gui_app, mocker):
    network = mocker.Mock()
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
        manager.handle_saveInitConfiguration(**info)
        network.onListInitConfigurations.assert_called_with(
            'XFEL/CAM/1')

    target = 'karabogui.singletons.manager.messagebox'
    mbox = mocker.patch(target)
    manager = Manager()
    info["success"] = False
    manager.handle_saveInitConfiguration(**info)
    mbox.show_error.assert_called_with(
        'Saving a configuration for XFEL/CAM/1 failed!', details='')


def test_handle_destinations(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        broadcast = mocker.patch(
            "karabogui.singletons.manager.broadcast_event")
        reply = Hash("destinations", ["one", "two", "three"])
        manager.handle_listDestinations(success=True,
                                        request=Hash(), reply=reply)
        broadcast.assert_called_with(
            KaraboEvent.ActiveDestinations, ["one", "two", "three"])


def test_handle_saveLogBook(mocker):
    logger = mocker.patch("karabogui.singletons.manager.get_logger")
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        args = Hash("dataType", "image")
        h = Hash("args", args)
        manager.handle_saveLogBook(success=True, request=h, reply=h)
        message = "Posted the image to LogBook successfully"
        logger().info.assert_called_with(message)
        assert logger().info.call_count == 1


def test_handle_onBeginTemporarySession(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        broadcast = mocker.patch(
            "karabogui.singletons.manager.broadcast_event")
        manager.handle_onBeginTemporarySession(success=False)
        assert broadcast.call_count == 0
        info = {"success": True, "accessLevel": 1}
        manager.handle_onBeginTemporarySession(**info)
        assert broadcast.call_count == 3
        first_call, second_call, third_call = broadcast.call_args_list
        args, _ = first_call
        assert args == (KaraboEvent.AccessLevelChanged, {})
        args, _ = second_call
        assert args == (KaraboEvent.LoginUserChanged, {})
        args, _ = third_call
        assert args == (KaraboEvent.UserSession, {})


def test_handle_onEndTemporarySession(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        broadcast = mocker.patch(
            "karabogui.singletons.manager.broadcast_event")

        manager.handle_onEndTemporarySession(levelBeforeTemporarySession=2,
                                             loggedUserId="karabo")
        assert broadcast.call_count == 3
        first_call, second_call, third_call = broadcast.call_args_list
        args, _ = first_call
        assert args == (KaraboEvent.AccessLevelChanged, {})
        args, _ = second_call
        assert args == (KaraboEvent.LoginUserChanged, {})
        args, _ = third_call
        assert args == (KaraboEvent.UserSession, {})


def test_handle_onTemporarySessionExpired(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        broadcast = mocker.patch(
            "karabogui.singletons.manager.broadcast_event")
        manager.handle_onTemporarySessionExpired(
            levelBeforeTemporarySession=2)
        assert broadcast.call_count == 2
        first_call, second_call = broadcast.call_args_list
        args, _ = first_call
        assert args == (KaraboEvent.LoginUserChanged, {})
        args, _ = second_call
        assert args == (KaraboEvent.UserSession, {})


def test_handle_onEndTemporarySessionNotice(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        broadcast = mocker.patch(
            "karabogui.singletons.manager.broadcast_event")
        manager.handle_onEndTemporarySessionNotice()
        assert broadcast.call_count == 1


def test_temp_session(mocker):
    network = mocker.Mock()
    with singletons(network=network):
        manager = Manager()
        login_info = {"username": "abcd", "accessLevel": 2}
        mocker.patch("karabogui.singletons.manager.broadcast_event")
        manager.handle_loginInformation(**login_info)

        assert krb_access.GLOBAL_ACCESS_LEVEL == AccessLevel(2)
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel(2)
        assert krb_access.TEMPORARY_SESSION_USER is None

        begin_session_info = {
            "username": "karabo", "accessLevel": 1, "success": True}
        manager.handle_onBeginTemporarySession(**begin_session_info)

        assert krb_access.GLOBAL_ACCESS_LEVEL == AccessLevel(1)
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel(1)
        assert krb_access.TEMPORARY_SESSION_USER == "karabo"

        end_session_info = {"levelBeforeTemporarySession": 2}
        manager.handle_onEndTemporarySession(**end_session_info)

        assert krb_access.GLOBAL_ACCESS_LEVEL == AccessLevel(2)
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel(2)
        assert krb_access.TEMPORARY_SESSION_USER is None

        temp_session_expired_info = {"levelBeforeTemporarySession": 1}
        manager.handle_onTemporarySessionExpired(**temp_session_expired_info)
        assert krb_access.GLOBAL_ACCESS_LEVEL == AccessLevel(1)
        assert krb_access.HIGHEST_ACCESS_LEVEL == AccessLevel(1)
        assert krb_access.TEMPORARY_SESSION_USER is None


def test_handle_session_info(mocker):
    target = 'karabogui.singletons.manager.broadcast_event'
    broadcast_event = mocker.patch(target)
    manager = Manager()
    info = Hash(
        "success", True, "reason", "",
        "type", "getGuiSessionInfo",
        "sessionStartTime", "2009-04-20T10:32:22 UTC",
        "sessionDuration", 120)

    manager.handle_getGuiSessionInfo(**info)
    data = {"sessionStartTime": "2009-04-20T10:32:22 UTC",
            "sessionDuration": 120}
    broadcast_event.assert_called_with(KaraboEvent.UserSessionInfo, data)
