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
from collections import OrderedDict
from unittest import main, mock

from qtpy.QtCore import QItemSelectionModel, QPoint, Qt

from karabo.common.api import AlarmCondition, State
from karabo.native import (
    AccessMode, Bool, Configurable, Float, Int32, Slot, String, VectorFloat,
    VectorHash)
from karabogui.binding.api import (
    DeviceProxy, ProjectDeviceProxy, ProxyStatus, apply_default_configuration,
    build_binding)
from karabogui.configurator.api import (
    ConfigurationTreeModel, ConfigurationTreeView, ConfiguratorFilterModel)
from karabogui.testing import GuiTestCase


class RowSchema(Configurable):
    start = Float()
    stop = Float()


class Object(Configurable):
    state = String(
        displayedName="State",
        enum=State,
        displayType="State",
        defaultValue=State.UNKNOWN,
        accessMode=AccessMode.READONLY)

    foo = Bool(
        displayedName="Foo",
        defaultValue=True)

    bar = Float(
        defaultValue=1.2,
        minInc=0.0,
        maxInc=10.0,
        warnHigh=0.1)

    baz = Int32()

    qux = String(
        options=["foo", "bar", "baz", "qux"])

    vector = VectorFloat(
        defaultValue=[1.2],
        minSize=1, maxSize=2)

    table = VectorHash(
        displayedName="Table",
        rows=RowSchema)

    alarmCondition = String(
        defaultValue=AlarmCondition.NONE,
        displayedName="Alarm",
        enum=AlarmCondition,
        displayType="AlarmCondition",
        accessMode=AccessMode.READONLY)

    @Slot(allowedStates=[State.INTERLOCKED, State.ACTIVE])
    def setSpeed(self):
        """Simple slot dummy method"""


class TestConfiguratorProjectDevice(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.view = ConfigurationTreeView()
        binding = build_binding(Object.getClassSchema())
        root = ProjectDeviceProxy(binding=binding, server_id="Test",
                                  status=ProxyStatus.OFFLINE)
        apply_default_configuration(root.binding)
        self.view.assign_proxy(proxy=root)
        self.model = self.view.model()
        assert isinstance(self.model, ConfigurationTreeModel)
        self.model.root = root
        self.model._config_update()

    def test_basics(self):
        # This is constant
        assert self.model.columnCount() == 3
        # The number of properties in the Object class up above
        assert self.model.rowCount() == 9

    def test_get_property_proxy_data(self):
        state_index = self.model.index(0, 0)
        assert state_index.data() == "State"
        assert state_index.data(role=Qt.ToolTipRole) is None

        bar_index = self.model.index(2, 0)
        assert bar_index.data() == "bar"
        text = "Key: bar - AccessLevel: USER - Access Allowed: True"
        assert bar_index.data(role=Qt.ToolTipRole) == text
        bar_index = self.model.index(2, 2)
        text = "Key: bar - AccessLevel: USER - Access Allowed: True"
        assert bar_index.data(role=Qt.ToolTipRole) == text

        proxy = self.model.property_proxy("bar")
        assert proxy.binding is not None
        assert proxy.value == 1.2
        bar_index = self.model.index(2, 1)
        assert bar_index.data() == "1.2"

        self.model.setData(bar_index, 3.2, role=Qt.EditRole)
        assert bar_index.data() == "3.2"
        self.model.setData(bar_index, 1.2, role=Qt.EditRole)
        assert bar_index.data() == "1.2"

        # Test via flushing, we have a project device
        proxy.edit_value = 5.2
        self.model.flush_index_modification(bar_index)
        assert bar_index.data() == "5.2"

        proxy.edit_value = 7.2
        self.model.clear_index_modification(bar_index)
        assert proxy.edit_value is None
        assert bar_index.data() == "5.2"

        assert bar_index.data(role=Qt.BackgroundRole) is None

    def test_attributes(self):
        bar_index = self.model.index(2, 0)
        assert bar_index.isValid()
        assert bar_index.data() == "bar"

        # no more attributes
        assert self.model.rowCount(parent=bar_index) == 0

        # ------------------------------------
        # Test the vector
        vector_index = self.model.index(5, 0)
        assert vector_index.isValid()
        assert vector_index.data() == "vector"
        # No attribute left
        assert self.model.rowCount(parent=vector_index) == 0

    def test_flags(self):
        bar_index = self.model.index(2, 0)
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled
        assert bar_index.flags() == flags

    def test_parent(self):
        bar_index = self.model.index(2, 0)
        assert not bar_index.parent().isValid()

    def test_swap(self):
        self.view.swap_models()
        assert isinstance(self.view.model(), ConfiguratorFilterModel)
        assert not self.view.model().dynamicSortFilter()
        self.view.swap_models()
        assert not isinstance(self.view.model(), ConfiguratorFilterModel)


class TestConfiguratorDevice(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.view = ConfigurationTreeView()
        binding = build_binding(Object.getClassSchema())
        root = DeviceProxy(binding=binding, server_id="Test",
                           status=ProxyStatus.ONLINE)
        apply_default_configuration(root.binding)
        self.view.assign_proxy(proxy=root)
        self.model = self.view.model()
        assert isinstance(self.model, ConfigurationTreeModel)
        self.model.root = root

    def test_get_property_proxy_data(self):
        state_index = self.model.index(0, 0)
        assert state_index.data() == "State"
        proxy = self.model.property_proxy("state")
        assert proxy.binding is not None
        assert proxy.value == "UNKNOWN"
        state_index = self.model.index(0, 1)
        assert state_index.data() == "UNKNOWN"
        brush = state_index.data(role=Qt.BackgroundRole)
        assert brush is not None
        color = brush.color()
        assert color.blue() == 0
        assert color.red() == 255
        assert color.green() == 170

        # Device updates from outside
        proxy.binding.trait_setq(value="ON")
        brush = state_index.data(role=Qt.BackgroundRole)
        assert state_index.data() == "ON"
        assert brush is not None
        color = brush.color()
        assert color.blue() == 0
        assert color.red() == 120
        assert color.green() == 255

        # Alarm coloring
        alarm_index = self.model.index(7, 0)
        assert alarm_index.data() == "Alarm"
        proxy = self.model.property_proxy("alarmCondition")
        assert proxy.binding is not None
        assert proxy.value == "none"
        alarm_index = self.model.index(7, 1)
        assert alarm_index.data() == "none"
        brush = alarm_index.data(role=Qt.BackgroundRole)
        assert brush is not None
        color = brush.color()
        assert color.blue() == 225
        assert color.red() == 225
        assert color.green() == 242

        # Test apply all and decline all, we have an online device
        proxy = self.model.property_proxy("bar")
        assert proxy.binding is not None
        assert proxy.value == 1.2
        bar_index = self.model.index(2, 1)
        assert bar_index.data() == "1.2"
        proxy.edit_value = 5.2
        path = "karabogui.configurator.qt_item_model.send_property_changes"
        with mock.patch(path) as changes:
            self.view.apply_all()
            changes.assert_called_once_with([proxy])

        self.view.decline_all()
        assert proxy.edit_value is None

        proxy.edit_value = 3.4
        self.view._reset_to_default(proxy)
        assert proxy.edit_value == 1.2

        # Test the context menu, we have an online device, but take an edit
        # index!
        edit_index = self.model.index(2, 2)
        self.view.selectionModel().setCurrentIndex(
            edit_index, QItemSelectionModel.ClearAndSelect)
        with mock.patch("karabogui.configurator.view.QMenu") as menu:
            self.view._show_context_menu(None)
            menu.assert_called_once()

        # Show the pop up widget
        event_pos = QPoint(20, 20)
        with mock.patch("karabogui.configurator.view.PopupWidget") as widget:
            self.view._show_popup_widget(self.model.index(0, 0), event_pos)
            widget.assert_called_once()
            widget().setInfo.assert_called_with(
                OrderedDict([('Property', 'State'), ('Key', 'state'),
                             ('Value Type', 'String'),
                             ('Default Value', 'UNKNOWN'),
                             ('AccessMode', 'READONLY'),
                             ('AccessLevel', 'OBSERVER'),
                             ('Assignment', 'OPTIONAL'),
                             ('Value on device', 'ON'),
                             ('metricPrefixSymbol', ''), ('unitSymbol', ''),
                             ('Warn low', 'n/a'), ('Warn high', 'n/a'),
                             ('Alarm low', 'n/a'), ('Alarm high', 'n/a'),
                             ('minExc', 'n/a'), ('maxExc', 'n/a'),
                             ('minInc', 'n/a'), ('maxInc', 'n/a'),
                             ('minSize', 'n/a'), ('maxSize', 'n/a'),
                             ('ArchivePolicy', 'n/a'),
                             ]))
            assert self.view.popup_widget is not None

    def test_modeltester_qt(self):
        from pytestqt.modeltest import ModelTester

        binding = build_binding(Object.getClassSchema())
        for proxy_type in (DeviceProxy, ProjectDeviceProxy):
            root = proxy_type(binding=binding, server_id="Test",
                              status=ProxyStatus.ONLINE)
            apply_default_configuration(root.binding)
            self.view.assign_proxy(proxy=root)
            self.model.root = root
            tester = ModelTester(None)
            tester.check(self.model)


if __name__ == "__main__":
    main()
