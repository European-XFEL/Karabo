from unittest.mock import Mock

from karabo.common.api import DeviceStatus
from karabo_gui.testing import assert_trait_change, singletons
from ..api import (
    ImageBinding, DeviceProxy, DeviceClassProxy, PropertyProxy,
    apply_default_configuration, build_binding
)
from .schema import get_pipeline_schema, get_simple_schema, get_slotted_schema


def test_device_proxy_classes():
    schema = get_simple_schema()
    binding = build_binding(schema)

    for factory in (DeviceProxy, DeviceClassProxy):
        proxy = factory(binding=binding)

        prop = proxy.get_property_binding('foo')
        assert prop is binding.value.foo

        not_there = proxy.get_property_binding('nope.blah')
        assert not_there is None


def test_device_class_proxy_status():
    topology = Mock()
    topology.get_attributes.return_value = None
    with singletons(topology=topology):
        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceClassProxy(server_id='swerver', binding=binding)

        assert proxy.status == DeviceStatus.NOSERVER
        topology.get_attributes.assert_called_with('server.swerver')

        topology.get_attributes.return_value = {}
        proxy = DeviceClassProxy(binding=binding)
        assert proxy.status == DeviceStatus.NOPLUGIN

        topology.get_attributes.return_value = {'deviceClasses': ['Simple']}
        proxy = DeviceClassProxy(binding=binding)
        assert proxy.status == DeviceStatus.OFFLINE

        proxy.status = DeviceStatus.REQUESTED
        proxy.schema_update = True
        assert proxy.status == DeviceStatus.OFFLINE
        assert not proxy.online


def test_device_class_proxy_refresh():
    network = Mock()
    with singletons(network=network):
        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceClassProxy(server_id='swerver', binding=binding)
        proxy.refresh_schema()

        assert proxy.status == DeviceStatus.REQUESTED
        network.onGetClassSchema.assert_called_with('swerver', 'Simple')


def test_device_proxy_slots():
    network = Mock()
    with singletons(network=network):
        schema = get_slotted_schema()
        binding = build_binding(schema)
        apply_default_configuration(binding)

        proxy = DeviceProxy(device_id='dev', server_id='swerver',
                            binding=binding)
        slot_proxy = PropertyProxy(path='callme', root_proxy=proxy)
        slot_proxy.execute()
        network.onExecute.assert_called_with('dev', 'callme')


def test_device_proxy_status():
    network = Mock()
    with singletons(network=network):
        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceProxy(device_id='dev', server_id='swerver',
                            binding=binding)

        assert proxy.status == DeviceStatus.OFFLINE
        proxy.add_monitor()
        proxy.status = DeviceStatus.ONLINE
        network.onGetDeviceSchema.assert_called_with('dev')

        proxy.schema_update = True
        network.onStartMonitoringDevice.assert_called_with('dev')
        assert proxy.status == DeviceStatus.SCHEMA

        proxy.config_update = True
        assert proxy.status == DeviceStatus.MONITORING

        proxy.remove_monitor()
        network.onStopMonitoringDevice.assert_called_with('dev')
        assert proxy.status == DeviceStatus.ALIVE

        proxy.schema_update = True
        network.onGetDeviceConfiguration.assert_called_with('dev')

        proxy.add_monitor()
        proxy.config_update = True
        assert proxy.status == DeviceStatus.MONITORING

        proxy.remove_monitor()
        proxy.status = DeviceStatus.ONLINE
        proxy.add_monitor()
        assert proxy.status == DeviceStatus.REQUESTED


def test_device_proxy_data_feeds():
    network = Mock()
    with singletons(network=network):
        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceProxy(device_id='dev', server_id='swerver',
                            binding=binding)

        proxy.connect_pipeline('pipe')
        network.onSubscribeToOutput.assert_called_with('dev', 'pipe', True)

        proxy.disconnect_pipeline('pipe')
        network.onSubscribeToOutput.assert_called_with('dev', 'pipe', False)

        historic_data = None

        def _event_fired(value):
            nonlocal historic_data
            historic_data = value

        apply_default_configuration(proxy.binding)
        binding = proxy.get_property_binding('foo')
        assert binding.value  # defaults to True

        binding.on_trait_change(_event_fired, 'historic_data')
        proxy.publish_historic_data('foo', False)
        assert historic_data is False


def test_property_proxy():
    schema = get_simple_schema()
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding)
    proxy = PropertyProxy(root_proxy=root_proxy, path='bar')
    other = PropertyProxy(root_proxy=root_proxy, path='foo')

    assert proxy.binding is binding.value.bar
    assert other.binding is binding.value.foo

    other.path = 'nope'
    assert other.binding is None

    assert other != proxy
    other.path = 'bar'
    assert other == proxy


def test_property_proxy_history():
    network = Mock()
    with singletons(network=network):
        schema = get_simple_schema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(device_id='dev', binding=binding)
        proxy = PropertyProxy(root_proxy=root_proxy, path='foo')

        proxy.get_history('start_time', 'end_time', max_value_count=1)
        network.onGetPropertyHistory.assert_called_with(
            'dev:foo', 'start_time', 'end_time', 1)


def test_property_proxy_pipeline():
    schema = get_pipeline_schema()
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding)
    proxy = PropertyProxy(root_proxy=root_proxy, path='output.data.image')

    assert isinstance(proxy.binding, ImageBinding)
    assert proxy.pipeline_parent_path == 'output'


def test_property_proxy_pipeline_monitoring():
    network = Mock()
    with singletons(network=network):
        schema = get_pipeline_schema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(device_id='dev', binding=binding)
        proxy = PropertyProxy(root_proxy=root_proxy, path='output.data.image')

        proxy.start_monitoring()
        network.onSubscribeToOutput.assert_called_with('dev', 'output', True)

        proxy.stop_monitoring()
        network.onSubscribeToOutput.assert_called_with('dev', 'output', False)


def test_schema_updates():
    schema = get_simple_schema()
    binding = build_binding(schema)
    proxy = DeviceProxy(binding=binding)

    with assert_trait_change(proxy, 'schema_update'):
        build_binding(schema, existing=proxy.binding)
