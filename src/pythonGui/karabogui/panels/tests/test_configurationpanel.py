from unittest.mock import patch

import numpy as np
from traits.api import Undefined

from karabo.common.api import ProxyStatus, State
from karabo.native import (
    AccessMode, Configurable, Float, Hash, HashList, String, UInt32,
    VectorBool, VectorUInt32)

from karabogui.binding.api import (
    apply_configuration, build_binding, DeviceProxy, ProjectDeviceProxy,
    validate_value)
from karabogui.testing import GuiTestCase

from ..configurationpanel import ConfigurationPanel, CONFIGURATION_PAGE


CONFIG_PANEL_PATH = "karabogui.panels.configurationpanel.ConfigurationPanel"
SHOW_NOT_LOADED_PROPERTIES_PATH = (CONFIG_PANEL_PATH
                                   + "._show_not_loaded_properties")

DEFAULT_VALUES = {
    "uintProperty": 2,
    "floatProperty": 2.0,
    "stringProperty": "foo",
    "vectorUint": [1, 2, 3],
    "vectorBool": [True, False, True],
}


class Object(Configurable):
    state = String(defaultValue=State.ON)

    uintProperty = UInt32()
    floatProperty = Float()
    stringProperty = String()

    vectorUint = VectorUInt32()
    vectorBool = VectorBool()

    uintReadOnly = UInt32(accessMode=AccessMode.READONLY)
    uintInitOnly = UInt32(accessMode=AccessMode.INITONLY)


class TestSetProxyConfiguration(GuiTestCase):

    def setUp(self):
        super(TestSetProxyConfiguration, self).setUp()
        # Setup online device proxy
        online_binding = build_binding(Object().getDeviceSchema())
        apply_configuration(Hash(DEFAULT_VALUES), online_binding)
        self.online_device_proxy = DeviceProxy(binding=online_binding,
                                               server_id='Test',
                                               status=ProxyStatus.ONLINE)
        # Setup offline device proxy
        offline_binding = build_binding(Object().getDeviceSchema())
        apply_configuration(Hash(DEFAULT_VALUES), offline_binding)
        self.offline_device_proxy = ProjectDeviceProxy(
            binding=offline_binding, server_id='Test',
            status=ProxyStatus.OFFLINE)

        self.config_panel = ConfigurationPanel()

    def tearDown(self):
        super(TestSetProxyConfiguration, self).tearDown()
        self.config_panel.destroy()
        self.config_panel = None

    def test_uint32_binding(self):
        self._assert_default_value("uintProperty")

        # -- Check with valid values -- #
        self._assert_config(uintProperty={"value": 22, "valid": True})
        # Check with a cast-able string
        self._assert_config(
            uintProperty={"value": "22",
                          "valid": True,
                          "casted": True})

        # -- Check with invalid values -- #
        self._assert_config(uintProperty={"value": -2, "valid": False})
        # Check with invalid uncast-able string
        self._assert_config(uintProperty={"value": "foo", "valid": False})
        # Check with floats. Should not pass, even with an integer-like value
        # XXX: Should it really not pass?
        self._assert_config(uintProperty={"value": 2.0, "valid": False})

    def test_float_binding(self):
        self._assert_default_value("floatProperty")

        # -- Check with valid values -- #
        self._assert_config(floatProperty={"value": 3.0, "valid": True})
        self._assert_config(floatProperty={"value": True, "valid": True})
        # Check with a cast-able string
        self._assert_config(
            floatProperty={"value": "3.33", "valid": True, "casted": True})

        # -- Check with invalid values -- #
        self._assert_config(floatProperty={"value": "foo", "valid": False})
        self._assert_config(floatProperty={"value": [1, 2, 3], "valid": False})

    def test_string_binding(self):
        self._assert_default_value("stringProperty")

        # -- Check with valid values -- #
        self._assert_config(
            stringProperty={"value": "bar", "valid": True})
        # Check with invalid but cast-able value
        self._assert_config(
            stringProperty={"value": 1, "valid": True, "casted": True})

        # -- Check with invalid values -- #
        self._assert_config(
            stringProperty={"value": None, "valid": False})

    def test_vector_uint32_binding(self):
        self._assert_default_value("vectorUint")

        # -- Check with valid values -- #
        self._assert_config(vectorUint={"value": [4, 5, 6], "valid": True})
        # Check with vector floats with integer values
        self._assert_config(vectorUint={"value": [4.0, 5.0, 6.0],
                                        "valid": True})

        # -- Check with invalid values -- #
        # Check with list that contains negative integers
        self._assert_config(vectorUint={"value": [-1, -2, 3], "valid": False})
        # Check with list that contains floats
        self._assert_config(vectorUint={"value": [4.4, 5.5, 6.6],
                                        "valid": False})

    def test_vector_bool_binding(self):
        self._assert_default_value("vectorBool")

        # -- Check with valid values -- #
        self._assert_config(
            vectorBool={"value": [False], "valid": True})
        # Check with vectors of 0s and 1s
        self._assert_config(
            vectorBool={"value": [0, 1, 1], "valid": True})

        # -- Check with invalid values -- #
        self._assert_config(vectorBool={"value": [1, 2, 3], "valid": False})
        self._assert_config(vectorBool={"value": "foo", "valid": False})

    def test_readonly_properties(self):
        # Check with uint read only value. Shouldn't trigger change
        self._assert_config(
            uintReadOnly={"value": 22, "valid": True, "changed": False})

    def test_initonly_properties(self):
        """In this test, the behavior of offline and online configurations are
           different. We have to separate the validation and provide
           corresponding expectations."""

        # Check uint property with initonly
        self._assert_online_config(
            uintInitOnly={"value": 22, "valid": True, "changed": False})
        self._assert_offline_config(
            uintInitOnly={"value": 22, "valid": True, "changed": True})

    def test_multiple_properties(self):
        # Check both valid values
        self._assert_config(
            uintProperty={"value": -2, "valid": False},
            floatProperty={"value": 3.0, "valid": True})

        # Check one invalid values
        self._assert_config(
            uintProperty={"value": 22, "valid": True},
            floatProperty={"value": "foo", "valid": False})

        # Check two invalid values
        self._assert_config(
            uintProperty={"value": 22, "valid": True},
            floatProperty={"value": "foo", "valid": False})

    # -----------------------------------------------------------------------
    # Assertions

    def _assert_config(self, **paths):
        self._assert_online_config(**paths)
        self._assert_offline_config(**paths)

    def _assert_online_config(self, **paths):
        # Set up config and changed values
        config, changed, validity = {}, {}, {}
        for path, values in paths.items():
            config[path] = values["value"]
            validity[path] = values["valid"]
            changed[path] = values.get("changed", values["valid"])

        # Apply changes
        with patch(SHOW_NOT_LOADED_PROPERTIES_PATH) as mocked_method:
            self._set_proxy_config(self.online_device_proxy, config)

        # Get invalid config
        invalid_config = {}
        for path, valid in validity.items():
            binding = self._get_online_property_binding(path)
            if binding is None:
                continue
            value = config[path]
            if validate_value(binding, value) is None:
                invalid_config[path] = value

        # Check if dialog is called
        if all(validity.values()):
            mocked_method.assert_not_called()
        else:
            mocked_method.assert_called_once_with(
                self.online_device_proxy, invalid_config)

        # Check edit value if changed
        for path, value in config.items():
            if not changed[path]:
                value = None
            casted = paths[path].get("casted", False)
            self._assert_edit_value(path, value, casted=casted)

    def _assert_offline_config(self, **paths):
        # Reset to default value
        binding = self.offline_device_proxy.binding
        apply_configuration(Hash(DEFAULT_VALUES), binding)

        # Set up config and changed values
        config, changed, validity = {}, {}, {}
        for path, values in paths.items():
            config[path] = values["value"]
            validity[path] = values["valid"]
            changed[path] = values.get("changed", values["valid"])

        # Apply changes
        with patch(SHOW_NOT_LOADED_PROPERTIES_PATH) as mocked_method:
            self._set_proxy_config(self.offline_device_proxy, config)

        # Get invalid config
        invalid_config = Hash()
        for path, valid in validity.items():
            binding = self._get_offline_property_binding(path)
            if binding is None:
                continue
            value = config[path]
            if validate_value(binding, value) is None:
                invalid_config[path] = value

        # Check if dialog is called
        if all(validity.values()):
            mocked_method.assert_not_called()
        else:
            mocked_method.assert_called_once_with(
                self.offline_device_proxy, invalid_config)

        # Check offline value if changed
        for path, value in config.items():
            if not changed[path]:
                value = DEFAULT_VALUES.get(path, Undefined)
            casted = paths[path].get("casted", False)
            self._assert_offline_value(path, value, casted=casted)

    def _assert_default_value(self, path):
        # Check with unchanged values
        self._assert_config(**{
            path: {"value": DEFAULT_VALUES[path],
                   "valid": True, "changed": False}
        })

    def _assert_edit_value(self, path, value, casted=False):
        edit_value = self._get_property_proxy(path).edit_value
        self._assert_value(actual=edit_value, expected=value, casted=casted)

    def _assert_offline_value(self, path, value, casted=False):
        actual_value = self._get_offline_property_binding(path).value
        self._assert_value(actual=actual_value, expected=value, casted=casted)

    def _assert_value(self, actual, expected, casted=False):
        if np.isscalar(expected):
            if casted and actual is not None:
                expected = type(actual)(expected)
            self.assertEqual(actual, expected)
        elif isinstance(actual, HashList):
            for actual_row, expected_row in zip(actual, expected):
                for path, actual_value in actual_row.items():
                    expected_value = expected_row[path]
                    self._assert_value(actual_value, expected_value, casted)
        else:
            np.testing.assert_array_equal(actual, expected)

    # -----------------------------------------------------------------------
    # Helper functions

    def _set_proxy_config(self, proxy, config):
        self.config_panel._show_configuration(proxy)
        self.config_panel._set_proxy_configuration(proxy, Hash(config))

    def _get_property_proxy(self, path):
        stacked_widget = self.config_panel._stacked_tree_widgets
        editor = stacked_widget.widget(CONFIGURATION_PAGE)
        model = editor.model()
        return model.property_proxy(path)

    def _get_online_property_binding(self, path):
        return self.online_device_proxy.get_property_binding(path)

    def _get_offline_property_binding(self, path):
        return self.offline_device_proxy.get_property_binding(path)
