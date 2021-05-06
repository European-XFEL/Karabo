from qtpy.QtCore import QModelIndex, Qt

from karabo.native import AccessLevel, Hash
from karabogui.navigation.system_filter_model import TopologyFilterModel
from karabogui.navigation.system_model import SystemTreeModel
from karabogui.testing import GuiTestCase, system_hash


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.system_model = SystemTreeModel()
        tree = self.system_model.tree
        tree.initialize(system_hash())
        self.model = TopologyFilterModel(self.system_model)

    def test_drag_actions(self):
        self.assertEqual(self.system_model.supportedDragActions(),
                         Qt.CopyAction)

    def test_header(self):
        header = self.system_model.headerData(0, Qt.Horizontal, Qt.DisplayRole)
        assert header == "Host - Server - Class - Device"

    def test_columnCount_root(self):
        self.assertEqual(self.system_model.columnCount(), 3)

    def test_columnCount_filter_model(self):
        self.assertEqual(self.model.columnCount(), 3)

    def test_rowCount_filter_model(self):
        self.assertEqual(self.model.rowCount(), 2)

    def test_rowCount_root(self):
        self.assertEqual(self.system_model.rowCount(), 2)

    def test_update_topo(self):
        """Add a device to the system model"""

        host_index = self.system_model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == 'BIG_IRON'

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
        h['device.newdevice'] = None
        h['device.newdevice', ...] = {
            'host': 'BIG_IRON',
            'visibility': AccessLevel.OBSERVER,
            'serverId': 'swerver',
            'classId': 'FooClass',
            'status': 'online'
        }
        self.system_model.tree.update(h)
        assert self.system_model.rowCount() == 2

        host_index = self.system_model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == 'BIG_IRON'

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
        assert host_index.data() == 'BIG_IRON'

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

    # ------------------------------------------------------------
    # Filter test

    def test_filter_model_device(self):
        """Test that we can filter in the system model"""
        self.model.setFilterFixedString("divvy")
        # Check that after correct filtering the device is there
        host_index = self.model.index(0, 0)
        assert host_index.isValid()
        assert host_index.data() == 'BIG_IRON'
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
