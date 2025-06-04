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
from qtpy.QtWidgets import QDialog

from karabo.native import AccessLevel, Hash
from karabogui.events import KaraboEvent
from karabogui.navigation.system_model import SystemTreeModel
from karabogui.navigation.system_view import SystemTreeView
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons, system_hash


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.view = SystemTreeView()
        self.model = self.view.model()
        self.system_model = self.model.sourceModel()
        tree = self.system_model.tree
        tree.initialize(system_hash())

    def tearDown(self):
        super().tearDown()
        self.view.destroy()
        self.model = None
        self.system_model = None
        self.view = None

    def test_view_click(self):
        self.assertTrue(self.view.expanded)
        self.view.onDoubleClickHeader()
        self.assertFalse(self.view.expanded)
        self.view.onDoubleClickHeader()
        self.assertTrue(self.view.expanded)

        # System model view is expanded on reset
        self.view.resetExpand()
        self.assertTrue(self.view.expanded)

        self.view.expandAll()
        self.assertTrue(self.view.expanded)
        self.view.collapseAll()
        self.assertFalse(self.view.expanded)

    def test_index_actions(self):
        selection_tracker = mock.Mock()
        model = self.model
        host_index = model.index(0, 0)
        server_index = model.index(0, 0, host_index)
        class_index = model.index(0, 0, server_index)
        device_index = model.index(0, 0, class_index)
        self.assertEqual(device_index.data(), "divvy")

        deviceId = ""
        navigation_type = ""
        device_proxy = None

        assert host_index.data() == "BIG_IRON"
        with singletons(mediator=Mediator()):
            call_slot = "karabogui.navigation.system_view.call_device_slot"
            is_online = "karabogui.navigation.system_view.is_device_online"
            with mock.patch(is_online) as ol:
                ol.return_value = True
                with mock.patch(call_slot) as call:
                    self.view.setCurrentIndex(host_index)
                    self.view.onNetworkInfo()
                    args = call.call_args.args
                    kwargs = call.call_args.kwargs
                    assert "BIG_IRON" in kwargs.values()
                    assert "requestNetwork" in args

        @Slot()
        def receiveSignal(nav_type, proxy):
            nonlocal navigation_type, deviceId, device_proxy
            navigation_type = nav_type
            deviceId = proxy.device_id if proxy is not None else None
            device_proxy = proxy

        model.signalItemChanged.connect(receiveSignal)

        index = device_index
        broadcast_path = "karabogui.navigation.system_view.broadcast_event"
        # Protect mediator in tests
        with singletons(selection_tracker=selection_tracker):
            with mock.patch(broadcast_path) as broadcast:
                model.selectIndex(index)
                self.process_qt_events()
                self.assertEqual(navigation_type, "device")
                self.assertEqual(deviceId, "divvy")
                broadcast.assert_called_with(KaraboEvent.ShowConfiguration,
                                             {"proxy": device_proxy})

        self.assertEqual(self.view.currentIndex(), index)
        self.assertTrue(self.view.currentIndex().isValid())
        # Check indexInfo
        info = self.view.indexInfo()
        self.assertEqual(info["deviceId"], "divvy")

        popup_widget = "karabogui.navigation.system_view.PopupWidget"
        with mock.patch(popup_widget) as widget:
            self.view.onAbout()
            widget().setInfo.assert_called_with(
                {"host": "BIG_IRON",
                 "archive": True,
                 "type": "device",
                 "visibility": AccessLevel.OBSERVER,
                 "capabilities": 5,
                 "interfaces": 0,
                 "serverId": "swerver",
                 "classId": "FooClass",
                 "status": "ok"})

        conf_widget = "karabogui.navigation.system_view." \
                      "ConfigurationFromPastDialog"
        with mock.patch(conf_widget) as widget:
            self.view.onGetConfigurationFromPast()
            widget.assert_called_once()

        conf_widget = "karabogui.navigation.system_view." \
                      "InitConfigurationDialog"
        with mock.patch(conf_widget) as widget:
            self.view.onGetConfigurationFromName()
            widget.assert_called_once()

        manager = mock.Mock()
        with singletons(manager=manager):
            with mock.patch("karabogui.singletons.manager.QMessageBox") as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                self.view.onKillInstance()
                manager.shutdownDevice.assert_called_once()

        manager.reset_mock()
        with singletons(manager=manager):
            scene_capa = "karabogui.navigation.system_view." \
                         "DeviceCapabilityDialog"
            with mock.patch(scene_capa) as scene:
                scene().device_id = "divvy"
                scene().capa_name = "scene"
                scene().exec.return_value = QDialog.Accepted
                self.view.onOpenDeviceScene()
                args = manager.callDeviceSlot.call_args[0]
                self.assertIn("requestScene", args)
                self.assertIn("divvy", args)
                manager.callDeviceSlot.assert_called_once()

        manager.reset_mock()
        with singletons(manager=manager):
            self.view.onTimeInformation()
            args = manager.callDeviceSlot.call_args[0]
            self.assertIn("divvy", args)
            self.assertIn("slotGetTime", args)
            manager.callDeviceSlot.assert_called_once()

        with singletons(selection_tracker=selection_tracker):
            with mock.patch(broadcast_path):
                model.selectIndex(server_index)

        manager.reset_mock()
        with singletons(manager=manager):
            with mock.patch("karabogui.singletons.manager.QMessageBox") as mb:
                mb().size.return_value = QSize(10, 10)
                mb().exec.return_value = mb.Yes
                self.view.onKillInstance()
                manager.shutdownServer.assert_called_once()
                # Servers don't have proxies
                self.assertEqual(device_proxy, None)

        network = mock.Mock()
        with singletons(network=network):
            path = "karabogui.navigation.system_view.QInputDialog"
            with mock.patch(path) as dia:
                dia.getItem.return_value = "DEBUG", True
                self.view.onLoggerLevel()
                network.onSetLogLevel.assert_called_with("swerver", "DEBUG")

    def test_context_menu_device_view(self):
        selection_tracker = mock.Mock()
        pos = QPoint(0, 0)
        menu_path = "karabogui.navigation.system_view.QMenu"
        with mock.patch(menu_path) as menu:
            view = SystemTreeView()
            model = view.model()
            host_index = model.index(0, 0)
            server_index = model.index(0, 0, host_index)
            class_index = model.index(0, 0, server_index)
            index = model.index(0, 0, class_index)
            broadcast_path = "karabogui.navigation.system_view.broadcast_event"
            # Protect mediator in tests
            with singletons(selection_tracker=selection_tracker):
                with mock.patch(broadcast_path):
                    model.selectIndex(index)
            view.onCustomContextMenuRequested(pos)
            menu().exec.assert_called_once()
            view.destroy()

    def test_drag_actions(self):
        self.assertEqual(self.system_model.supportedDragActions(),
                         Qt.CopyAction)

    def test_header(self):
        header = self.system_model.headerData(0, Qt.Horizontal, Qt.DisplayRole)
        assert header == "Host - Server - Class - Device"

    def test_columnCount_root(self):
        self.assertEqual(self.system_model.columnCount(), 2)

    def test_columnCount_filter_model(self):
        self.assertEqual(self.model.columnCount(), 2)

    def test_rowCount_filter_model(self):
        self.assertEqual(self.model.rowCount(), 1)

    def test_rowCount_root(self):
        self.assertEqual(self.system_model.rowCount(), 1)

    def test_dynamics_sorting(self):
        assert not self.model.dynamicSortFilter()

    def test_update_topo(self):
        """Add a device to the system model"""

        host_index = self.system_model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == "BIG_IRON"

        host = host_index.internalPointer()
        # We have 1 server on host
        assert len(host.children) == 1

        server_index = self.system_model.index(0, 0, host_index)
        server = server_index.internalPointer()
        # We have 2 classes
        assert len(server.children) == 2

        class_index = self.system_model.index(0, 0, server_index)
        class_node = class_index.internalPointer()
        assert len(class_node.children) == 1

        # We only have 1 device
        device_index = self.system_model.index(0, 0, class_index)
        assert device_index.data() == "divvy"

        # We add a single device with same class
        h = Hash()
        h["device.newdevice"] = None
        h["device.newdevice", ...] = {
            "host": "BIG_IRON",
            "visibility": AccessLevel.OBSERVER,
            "serverId": "swerver",
            "classId": "FooClass",
            "status": "ok",
            "capabilities": 0,
        }
        self.system_model.tree.update(h)
        # __none__ server is not accounted anymore ...
        assert self.system_model.rowCount() == 1

        host_index = self.system_model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == "BIG_IRON"

        host = host_index.internalPointer()
        assert len(host.children) == 1

        # servers
        server_index = self.system_model.index(0, 0, host_index)
        server = server_index.internalPointer()
        assert len(server.children) == 2

        # We now have two devices
        class_index = self.system_model.index(0, 0, server_index)
        class_node = class_index.internalPointer()
        assert len(class_node.children) == 2

        device_index = self.system_model.index(0, 0, class_index)
        assert device_index.data() == "divvy"

        device_index = self.system_model.index(1, 0, class_index)
        assert device_index.data() == "newdevice"

    def test_update_topo_remove_device(self):
        """Remove a device from the system model"""
        self.system_model.tree.remove_device("newdevice")

        host_index = self.system_model.index(0, 0)
        assert host_index.data() == "BIG_IRON"

        host = host_index.internalPointer()
        assert len(host.children) == 1

        # servers
        server_index = self.system_model.index(0, 0, host_index)
        server = server_index.internalPointer()
        assert len(server.children) == 2

        # We removed the added device
        class_index = self.system_model.index(0, 0, server_index)
        assert class_index.data() == "FooClass"
        class_node = class_index.internalPointer()
        assert len(class_node.children) == 1

        device_index = self.system_model.index(0, 0, class_index)
        assert device_index.data() == "divvy"

    def test_find_nodes(self):
        """Test that we can find a node"""
        node = self.model.findNodes("divvy")
        assert len(node)

    # ------------------------------------------------------------
    # Filter test

    def test_filter_model_device(self):
        """Test that we can filter in the system model"""
        self.model.setFilterFixedString("divvy")
        # Check that after correct filtering the device is there
        host_index = self.model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == "BIG_IRON"
        server_index = self.model.index(0, 0, host_index)
        class_index = self.model.index(0, 0, server_index)
        device_index = self.model.index(0, 0, class_index)
        assert device_index.data() == "divvy"

        self.model.setFilterFixedString("xfel")
        host_index = self.model.index(0, 0)
        # Host is not valid, filtered out
        assert not host_index.isValid()

    # ------------------------------------------------------------
    # Basic model stress test

    def test_data_bad_index(self):
        self.assertIsNone(self.system_model.data(QModelIndex(),
                                                 Qt.DisplayRole))

    def test_modeltester_qt(self):
        from pytestqt.modeltest import ModelTester
        system_model = SystemTreeModel()
        tree = system_model.tree
        tree.initialize(system_hash())
        tester = ModelTester(None)
        tester.check(system_model)


if __name__ == "__main__":
    main()
