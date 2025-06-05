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
from unittest import main, mock

from qtpy.QtCore import QModelIndex, QPoint, QSize, Qt, Slot

from karabo.native import AccessLevel, Hash
from karabogui.events import KaraboEvent
from karabogui.navigation.device_model import DeviceTreeModel
from karabogui.navigation.device_view import DeviceTreeView
from karabogui.testing import GuiTestCase, device_hash, singletons


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.view = DeviceTreeView()
        self.model = self.view.model()
        self.device_model = self.model.sourceModel()
        tree = self.device_model.tree
        tree.initialize(device_hash())

    def tearDown(self):
        super().tearDown()
        self.view.destroy()
        self.view = None

    def test_view_click(self):
        self.assertFalse(self.view.expanded)
        self.view.onDoubleClickHeader()
        self.assertTrue(self.view.expanded)
        self.view.onDoubleClickHeader()
        self.assertFalse(self.view.expanded)
        self.view.onDoubleClickHeader()
        self.assertTrue(self.view.expanded)

        # Device tree view is collapsed on reset
        self.view.resetExpand()
        self.assertFalse(self.view.expanded)

        self.view.expandAll()
        self.assertTrue(self.view.expanded)
        self.view.collapseAll()
        self.assertFalse(self.view.expanded)

    def test_index_actions(self):
        selection_tracker = mock.Mock()
        model = self.model
        domain_index = model.index(0, 0, QModelIndex())
        foo_type_index = model.index(0, 0, domain_index)
        member_index = model.index(0, 0, foo_type_index)
        self.assertEqual(member_index.data(), "XFEL/FOO/1")

        index = member_index
        broadcast_path = "karabogui.navigation.device_view.broadcast_event"
        # Protect mediator in tests
        with singletons(selection_tracker=selection_tracker):
            with mock.patch(broadcast_path):
                model.selectIndex(index)

        deviceId = ""
        navigation_type = ""
        device_proxy = None

        @Slot()
        def receiveSignal(nav_type, proxy):
            nonlocal navigation_type, deviceId, device_proxy
            navigation_type = nav_type
            deviceId = proxy.device_id
            device_proxy = proxy

        model.signalItemChanged.connect(receiveSignal)
        self.assertEqual(self.view.currentIndex(), index)
        self.assertTrue(self.view.currentIndex().isValid())
        # Check indexInfo
        info = self.view.indexInfo()
        self.assertEqual(info["deviceId"], "XFEL/FOO/1")

        # Check events and selection!
        with singletons(selection_tracker=selection_tracker):
            with mock.patch(broadcast_path) as broadcast:
                member_index = model.index(1, 0, foo_type_index)
                model.selectIndex(member_index)
                self.process_qt_events()
                self.assertEqual(navigation_type, "device")
                self.assertEqual(deviceId, "XFEL/FOO/2")
                broadcast.assert_called_with(KaraboEvent.ShowConfiguration,
                                             {"proxy": device_proxy})

        popup_widget = "karabogui.navigation.device_view.PopupWidget"
        with mock.patch(popup_widget) as widget:
            self.view.onAbout()
            widget().setInfo.assert_called_with(
                {"host": "BIG_IRON",
                 "archive": True,
                 "visibility": AccessLevel.OBSERVER,
                 "capabilities": 5,
                 "serverId": "swerver",
                 "classId": "FooClass",
                 "interfaces": 0,
                 "status": "ok"})

        conf_widget = "karabogui.navigation.device_view." \
                      "ConfigurationFromPastDialog"
        with mock.patch(conf_widget) as widget:
            self.view.onGetConfigurationFromPast()
            widget.assert_called_once()

        conf_widget = "karabogui.navigation.device_view." \
                      "InitConfigurationDialog"
        with mock.patch(conf_widget) as widget:
            self.view.onGetInitConfiguration()
            widget.assert_called_once()

        manager = mock.Mock()
        with singletons(manager=manager):
            with mock.patch('karabogui.singletons.manager.QMessageBox') as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                self.view.onKillInstance()
                manager.shutdownDevice.assert_called_once()

    def test_context_menu_device_view(self):
        pos = QPoint(0, 0)
        selection_tracker = mock.Mock()
        menu_path = "karabogui.navigation.device_view.QMenu"
        with mock.patch(menu_path) as menu:
            view = DeviceTreeView()
            model = view.model()
            domain_index = model.index(0, 0, QModelIndex())
            foo_type_index = model.index(0, 0, domain_index)
            index = model.index(0, 0, foo_type_index)
            broadcast_path = "karabogui.navigation.device_view.broadcast_event"
            # Protect mediator in tests
            with singletons(selection_tracker=selection_tracker):
                with mock.patch(broadcast_path):
                    model.selectIndex(index)
            view.onCustomContextMenuRequested(pos)
            menu().exec.assert_called_once()
            view.destroy()

    def test_drag_actions(self):
        self.assertEqual(self.device_model.supportedDragActions(),
                         Qt.CopyAction)

    def test_header(self):
        header = self.device_model.headerData(0, Qt.Horizontal, Qt.DisplayRole)
        assert header == "Domain - Type - Name"

    def test_columnCount_root(self):
        self.assertEqual(self.device_model.columnCount(), 1)

    def test_columnCount_filter_model(self):
        self.assertEqual(self.model.columnCount(), 1)

    def test_rowCount_filter_model(self):
        self.assertEqual(self.model.rowCount(), 1)

    def test_rowCount_root(self):
        self.assertEqual(self.device_model.rowCount(), 1)

    def test_dynamics_sorting(self):
        assert not self.model.dynamicSortFilter()

    def test_update_topo(self):
        """Add a device to the system model"""

        domain_index = self.device_model.index(0, 0)
        domain = domain_index.internalPointer()
        assert domain_index.data() == "XFEL"
        # Domain XFEL has two children
        assert len(domain.children) == 2

        foo_type_index = self.device_model.index(0, 0, domain_index)
        foo_node = foo_type_index.internalPointer()
        # We have two devices under
        assert len(foo_node.children) == 2
        assert foo_type_index.data() == "FOO"

        bar_type_index = self.device_model.index(1, 0, domain_index)
        bar_node = bar_type_index.internalPointer()
        assert bar_type_index.data() == "BAR"
        assert len(bar_node.children) == 1

        member_index = self.device_model.index(0, 0, foo_type_index)
        assert member_index.data() == "XFEL/FOO/1"
        member_index = self.device_model.index(1, 0, foo_type_index)
        assert member_index.data() == "XFEL/FOO/2"

        member_index = self.device_model.index(0, 0, bar_type_index)
        assert member_index.data() == "XFEL/BAR/1"

        h = Hash()
        h["device.XFEL/BAR/2"] = None
        h["device.XFEL/BAR/2", ...] = {
            "host": "BIG_IRON",
            "visibility": AccessLevel.OBSERVER,
            "serverId": "swerver",
            "classId": "BarClass",
            "status": "ok",
            "interfaces": 0,
            "capabilities": 0,
        }
        self.device_model.tree.update(h)

        bar_type_index = self.device_model.index(1, 0, domain_index)
        bar_node = bar_type_index.internalPointer()
        assert bar_type_index.data() == "BAR"
        # Additional device under BAR
        assert len(bar_node.children) == 2
        member_index = self.device_model.index(0, 0, bar_type_index)
        assert member_index.data() == "XFEL/BAR/1"
        member_index = self.device_model.index(1, 0, bar_type_index)
        assert member_index.data() == "XFEL/BAR/2"

    def test_update_topo_remove_device(self):
        """Remove a device from the device model"""
        self.device_model.tree.remove_device("XFEL/BAR/2")

        domain_index = self.device_model.index(0, 0)
        bar_type_index = self.device_model.index(1, 0, domain_index)
        bar_node = bar_type_index.internalPointer()
        assert bar_type_index.data() == "BAR"
        # Removed device under BAR
        assert len(bar_node.children) == 1
        member_index = self.device_model.index(0, 0, bar_type_index)
        assert member_index.data() == "XFEL/BAR/1"

    # ------------------------------------------------------------
    # Filter test

    def test_filter_model_device(self):
        """Test that we can filter in the device model"""
        self.model.setFilterFixedString("FOO/1")
        # Check that after correct filtering the device is there
        domain_index = self.model.index(0, 0)
        assert domain_index.isValid()
        assert domain_index.data() == "XFEL"
        foo_type_index = self.model.index(0, 0, domain_index)
        assert foo_type_index.data() == "FOO"
        # Only 1 device left after filtering
        device_index = self.model.index(0, 0, foo_type_index)
        assert device_index.data() == "XFEL/FOO/1"

        # No more valid children!
        device_index = self.model.index(1, 0, foo_type_index)
        assert not device_index.isValid()
        empty_node = device_index.internalPointer()
        assert empty_node is None

        # Completely empty, no interface
        self.model.setFilterSelection("Motor")
        self.model.setFilterFixedString("")
        domain_index = self.model.index(0, 0)
        assert not domain_index.isValid()

    # ------------------------------------------------------------
    # Basic model stress test

    def test_data_bad_index(self):
        self.assertIsNone(self.device_model.data(QModelIndex(),
                                                 Qt.DisplayRole))

    def test_modeltester_qt(self):
        from pytestqt.modeltest import ModelTester
        device_model = DeviceTreeModel()
        tree = device_model.tree
        tree.initialize(device_hash())
        tester = ModelTester(None)
        tester.check(device_model)


if __name__ == "__main__":
    main()
