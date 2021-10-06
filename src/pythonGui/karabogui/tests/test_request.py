from unittest import TestCase, main, mock

from karabo.common.api import ProxyStatus
from karabo.native import Hash
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, apply_configuration,
    apply_default_configuration, build_binding)
from karabogui.binding.tests.schema import get_simple_schema
from karabogui.request import onConfigurationUpdate, onSchemaUpdate
from karabogui.testing import singletons


class TestRequestModule(TestCase):

    def setUp(self):
        schema = get_simple_schema()
        root_binding = build_binding(schema)
        apply_default_configuration(root_binding)
        self.root_proxy = DeviceProxy(binding=root_binding, device_id="marty")
        self.root_proxy.status = ProxyStatus.ONLINE
        self.property_proxy = PropertyProxy(
            root_proxy=self.root_proxy, path='foo')

    def test_config_update(self):
        received = 0

        def handler():
            nonlocal received
            received += 1

        for proxy in (self.root_proxy, self.property_proxy):
            received = 0

            onConfigurationUpdate(proxy, handler)

            config = Hash("foo", True, "bar", "a", "charlie", 0)
            apply_configuration(config, self.root_proxy.binding)
            self.assertEqual(received, 1)
            apply_configuration(config, self.root_proxy.binding)
            self.assertEqual(received, 1)

            handler = onConfigurationUpdate(proxy, handler, remove=False)
            apply_configuration(config, self.root_proxy.binding)
            self.assertEqual(received, 2)
            apply_configuration(config, self.root_proxy.binding)
            self.assertEqual(received, 3)

            # Remove manually
            proxy.binding.on_trait_change(handler, 'config_update',
                                          remove=True)
            apply_configuration(config, self.root_proxy.binding)
            self.assertEqual(received, 3)

            network = mock.Mock()
            with singletons(network=network):
                onConfigurationUpdate(proxy, handler, request=True)
                apply_configuration(config, self.root_proxy.binding)
                self.assertEqual(received, 4)
                network.onGetDeviceConfiguration.assert_called_with("marty")

    def test_schema_update(self):
        received = 0

        def handler():
            nonlocal received
            received += 1

        for proxy in (self.root_proxy, self.property_proxy):
            received = 0

            onSchemaUpdate(proxy, handler)

            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            self.assertEqual(received, 1)
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            self.assertEqual(received, 1)

            handler = onSchemaUpdate(proxy, handler, remove=False)
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            self.assertEqual(received, 2)

            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            self.assertEqual(received, 3)

            # Remove manually
            self.root_proxy.on_trait_change(handler, 'schema_update',
                                            remove=True)
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            self.assertEqual(received, 3)

            network = mock.Mock()
            with singletons(network=network):
                onSchemaUpdate(proxy, handler, request=True, remove=True)
                build_binding(get_simple_schema(),
                              existing=self.root_proxy.binding)
                network.onGetDeviceSchema.assert_called_with("marty")


if __name__ == "__main__":
    main()
