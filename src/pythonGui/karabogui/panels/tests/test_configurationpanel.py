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

import numpy as np
from traits.api import Undefined

from karabo.common.api import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, Float, Hash,
    HashList, String, UInt32, VectorBool, VectorHash, VectorUInt32)
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, ProjectDeviceProxy, ProxyStatus,
    VectorHashBinding, apply_configuration, apply_default_configuration,
    build_binding, validate_table_value, validate_value)
from karabogui.singletons.mediator import Mediator
from karabogui.testing import (
    GuiTestCase, access_level, click_button, get_all_props_schema, singletons)

from ..configurationpanel import CONFIGURATION_PAGE, ConfigurationPanel
from ..panel_info import create_configurator_info

CONFIG_PANEL_PATH = "karabogui.panels.configurationpanel.ConfigurationPanel"
SHOW_NOT_LOADED_PROPERTIES_PATH = (CONFIG_PANEL_PATH
                                   + "._show_not_loaded_properties")

DEFAULT_VALUES = {
    "uintProperty": 2,
    "floatProperty": 2.0,
    "stringProperty": "foo",
    "vectorUint": [1, 2, 3],
    "vectorBool": [True, False, True],
    "vectorHash": [Hash('uintProperty', 1,
                        'stringProperty', 'spam',
                        'floatProperty', 5.0)],
    "vectorHashReadOnlyColumn": [Hash('uintProperty', 1,
                                      'floatProperty', 5.0)],
    "vectorHashInitOnlyColumn": [Hash('uintProperty', 1,
                                      'floatProperty', 5.0)]}


class TableRow(Configurable):
    uintProperty = UInt32(defaultValue=1)
    stringProperty = String(defaultValue="spam")
    floatProperty = Float(defaultValue=5.0)


class TableRowEmpty(Configurable):
    uintProperty = UInt32()
    stringProperty = String()
    floatProperty = Float()


class TableRowReadOnly(Configurable):
    uintProperty = UInt32(defaultValue=1, accessMode=AccessMode.READONLY)
    floatProperty = Float(defaultValue=5.0)


class TableRowInitOnly(Configurable):
    uintProperty = UInt32(defaultValue=1, accessMode=AccessMode.INITONLY)
    floatProperty = Float(defaultValue=5.0)


class Object(Configurable):
    state = String(defaultValue=State.ON)

    uintProperty = UInt32()
    floatProperty = Float()
    stringProperty = String()

    vectorUint = VectorUInt32()
    vectorBool = VectorBool()

    uintReadOnly = UInt32(accessMode=AccessMode.READONLY)
    uintInitOnly = UInt32(accessMode=AccessMode.INITONLY)

    internal = Bool(
        defaultValue=True,
        assignment=Assignment.INTERNAL)

    # -----------------------------------------------------------------------
    # Disable the MDL sanitization for tables in tests

    def new_sanitize(schema, ro):
        return schema

    with patch('karabo.native.schema.descriptors.sanitize_table_schema',
               new=new_sanitize):
        vectorHash = VectorHash(rows=TableRow)
        vectorHashReadOnly = VectorHash(rows=TableRow,
                                        accessMode=AccessMode.READONLY)
        vectorHashInitOnly = VectorHash(rows=TableRow,
                                        accessMode=AccessMode.INITONLY)
        vectorHashReadOnlyColumn = VectorHash(rows=TableRowReadOnly)
        vectorHashInitOnlyColumn = VectorHash(rows=TableRowInitOnly)
        vectorHashEmpty = VectorHash(TableRowEmpty)


class TestSetProxyConfiguration(GuiTestCase):

    def setUp(self):
        super().setUp()
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
        super().tearDown()
        self.config_panel._reset_panel()
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

    def test_valid_vector_hash(self):
        # Check setting the default value again.. It shouldn't trigger change.
        self._assert_default_value("vectorHash")

        # Check with only one value changed. Should trigger a change.
        value = [Hash('uintProperty', 2,  # this changed
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]
        self._assert_config(vectorHash={"value": value, "valid": True})

        # Check with values as strings. Should still be a valid change.
        value = [Hash('uintProperty', '2',
                      'stringProperty', 'spam',
                      'floatProperty', '5.0')]
        self._assert_config(
            vectorHash={"value": value, "valid": True, "casted": True})

        # Reordering the default value is a valid input but should not
        # trigger change
        value = [Hash('stringProperty', 'spam',
                      'uintProperty', 1,
                      'floatProperty', 5.0)]
        self._assert_config(
            vectorHash={"value": value, "valid": True, "changed": False})

        # Check multiple rows: one reordered Hash, one new row
        value = [Hash('stringProperty', 'spam',
                      'uintProperty', 1,
                      'floatProperty', 5.0),
                 Hash('uintProperty', 4,
                      'stringProperty', 'foobarbar',
                      'floatProperty', 5.6)]
        self._assert_config(
            vectorHash={"value": value, "valid": True})

        # Check additional columns that are not on the schema. It is ignored.
        # We use the default value as control, it shouldn't trigger change.
        value = [Hash('stringProperty', 'spam',
                      'uintProperty', 1,
                      'floatProperty', 5.0,
                      'nonExistentProperty', 0)]
        self._assert_config(
            vectorHash={"value": value, "valid": True, "changed": False})

        # Check comparison with empty vector hash
        value = [Hash('uintProperty', 1,  # this changed
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]
        self._assert_config(vectorHashEmpty={"value": value, "valid": True})

    def test_invalid_vector_hash(self):
        # Check with only one invalid value changed.
        # Should not trigger change as the rest are default values
        value = [Hash('uintProperty', -2,  # invalid value for a uint
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]
        self._assert_config(
            vectorHash={"value": value, "valid": False, "changed": False})

        # Check with changes with one invalid value
        value = [Hash('uintProperty', -2,  # invalid value for a uint
                      'stringProperty', 'foo',  # changed
                      'floatProperty', 5.5)]  # changed
        self._assert_config(
            vectorHash={"value": value, "valid": False, "changed": False})

        # Check with changes with all invalid value
        value = [Hash('uintProperty', -2,  # invalid value for a uint
                      'stringProperty', None,  # invalid for a string
                      'floatProperty', 'foo')]  # invalid for a float
        self._assert_config(
            vectorHash={"value": value, "valid": False, "changed": False})

        # Check comparison with empty vector hash
        value = [Hash('uintProperty', -2,  # invalid value for a uint
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]
        self._assert_config(
            vectorHashEmpty={"value": value, "valid": False, "changed": False})

    def test_readonly_properties(self):
        # Check with uint read only value. Shouldn't trigger change
        self._assert_config(
            uintReadOnly={"value": 22, "valid": True, "changed": False})

        # Check with a readonly table element. Shouldn't trigger changes.
        value = [Hash('uintProperty', 4,  # new values
                      'floatProperty', 5.6)]  # changed, and valid
        self._assert_config(
            vectorHashReadOnly={"value": value,
                                "valid": True,
                                "changed": False})

        # Check with a reconfigurable table element with a read-only column
        # by setting a changed data on the read-only column and unchanged on
        # the reconfigurable column. This WILL trigger changes!
        value = [Hash('uintProperty', 4,  # changed, but read only
                      'floatProperty', 5.0)]  # not changed
        self._assert_config(
            vectorHashReadOnlyColumn={"value": value,
                                      "valid": True,
                                      "changed": True})

        # Check with a reconfigurable table element with a read-only column
        # by setting a changed data on both the read-only and reconfigurable
        # Should trigger changes.
        value = [Hash('uintProperty', 4,  # changed, but read only
                      'floatProperty', 6.0)]  # changed
        self._assert_config(
            vectorHashReadOnlyColumn={"value": value,
                                      "valid": True,
                                      "changed": True})

        # Check comparison with incomplete vector hash (without default value).
        value = [Hash('uintProperty', 1,  # this changed
                      'floatProperty', 5.0)]
        self._assert_config(vectorHashEmpty={"value": value,
                                             "valid": False,
                                             "changed": False})

    def test_initonly_properties(self):
        """In this test, the behavior of offline and online configurations are
           different. We have to separate the validation and provide
           corresponding expectations."""

        # Check uint property with initonly
        self._assert_online_config(
            uintInitOnly={"value": 22, "valid": True, "changed": False})
        self._assert_offline_config(
            uintInitOnly={"value": 22, "valid": True, "changed": True})

        # Check assignment internal with bool
        # -> changed when online and reconfigurable
        # -> not changed when offline and reconfigurable
        self._assert_online_config(
            internal={"value": False, "valid": True, "changed": True})
        self._assert_offline_config(
            internal={"value": False, "valid": True, "changed": False})

        # Check with an init-only table element. Shouldn't trigger changes on
        # online device but should trigger change on offline device.
        value = [Hash('uintProperty', 4,  # new value
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]  # new value
        self._assert_online_config(
            vectorHashInitOnly={"value": value,
                                "valid": True,
                                "changed": False})
        self._assert_offline_config(
            vectorHashInitOnly={"value": value,
                                "valid": True,
                                "changed": True})

        # Check with a reconfigurable table element with a init-only column
        # by setting a changed data on the init-only column and unchanged on
        # the reconfigurable column. Shouldn't trigger changes on online device
        # but should change on offline device.
        value = [Hash('uintProperty', 4,  # new value
                      'stringProperty', 'spam',
                      'floatProperty', 5.0)]  # new value
        self._assert_online_config(
            vectorHashInitOnly={"value": value,
                                "valid": True,
                                "changed": False})
        self._assert_offline_config(
            vectorHashInitOnly={"value": value,
                                "valid": True,
                                "changed": True})

        # Check with a reconfigurable table element with a init-only column
        # by setting a changed data on both the init-only and reconfigurable
        # column Should trigger changes both on online and offline devices.
        value = [Hash('uintProperty', 4,  # changed, but init only
                      'floatProperty', 6.0)]  # not changed
        self._assert_online_config(
            vectorHashInitOnlyColumn={"value": value,
                                      "valid": True,
                                      "changed": True})
        self._assert_offline_config(
            vectorHashInitOnlyColumn={"value": value,
                                      "valid": True,
                                      "changed": True})

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
            if isinstance(binding, VectorHashBinding):
                valid, invalid = validate_table_value(binding, value)
                config[path] = valid
                invalid_config[path] = invalid
            elif validate_value(binding, value) is None:
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
            if isinstance(binding, VectorHashBinding):
                valid, invalid = validate_table_value(
                    binding, value)
                config[path] = valid
                invalid_config[path] = invalid
            elif validate_value(binding, value) is None:
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
            assert actual == expected
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


def test_configuration_panel_info(gui_app):
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert binding.value.a.value is Undefined
    assert binding.value.b.value is Undefined
    assert binding.value.c.value is Undefined

    apply_default_configuration(binding)
    assert binding.value.a.value
    assert binding.value.b.value == "c"
    assert binding.value.c.value is Undefined

    mediator = Mediator()
    with singletons(mediator=mediator):
        proxy = DeviceProxy(binding=binding,
                            server_id="Test",
                            status=ProxyStatus.ONLINE)

        panel = ConfigurationPanel()
        # XXX: Make to see all
        panel._switch_mode_toggled(True)
        panel.model().root = proxy
        panel._set_configuration(proxy)
        # Make sure the extracted default conversion is minimal
        # It should include properties with default values,
        # options, or node types
        with access_level(AccessLevel.EXPERT):
            config = create_configurator_info(panel)

        assert isinstance(config, Hash)

        default_props = ("a", "b", "h1")
        for prop in default_props:
            assert config[prop] != "<undefined>"

        # List and Choice of Node and not present
        assert "i1" not in config
        assert "j1" not in config

        # Slot k1 is not present
        assert "k1" not in config

        # m has no default value, but present with Undefined
        assert "m" in config
        assert config["m"] == "<undefined>"

        assert "c" in config
        assert config["c"] == "<undefined>"


def test_configuration_from_past(gui_app, mocker):

    panel = ConfigurationPanel()
    assert not panel.ui_history_config.isVisible()

    proxy_binding = build_binding(Object().getDeviceSchema())
    apply_configuration(Hash(DEFAULT_VALUES), proxy_binding)
    proxy = DeviceProxy(
        binding=proxy_binding, server_id='Test', status=ProxyStatus.ONLINE)
    panel._set_configuration(proxy)
    assert panel.ui_history_config.isVisible()

    # Make sure the button shows the ConfigurationFromPastDialog
    toolbar = panel.ui_history_config.parent()
    button = toolbar.widgetForAction(panel.ui_history_config)
    assert button.toolTip() == "Configuration from past"
    mock_dialog = mocker.patch(
        "karabogui.panels.configurationpanel.ConfigurationFromPastDialog")
    click_button(button)
    assert mock_dialog().show.call_count == 1

    # Test the visibility of toolbutton.
    # Offline Device
    proxy = DeviceProxy(
        binding=proxy_binding, status=ProxyStatus.OFFLINE)
    panel._set_configuration(proxy)
    assert panel.ui_history_config.isVisible()

    # Device class proxy
    proxy = DeviceClassProxy(binding=proxy_binding,)
    panel._set_configuration(proxy)
    assert not panel.ui_history_config.isVisible()

    # Project Device Proxy
    proxy = ProjectDeviceProxy(binding=proxy_binding)
    panel._set_configuration(proxy)
    assert panel.ui_history_config.isVisible()
