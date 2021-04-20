from qtpy.QtCore import QModelIndex, Qt

from karabo.native import AccessLevel, Hash
from karabogui.testing import GuiTestCase, device_hash
from karabogui.navigation.device_model import DeviceTreeModel
from karabogui.navigation.device_filter_model import DeviceFilterModel


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.device_model = DeviceTreeModel()
        tree = self.device_model.tree
        tree.initialize(device_hash())
        self.model = DeviceFilterModel(self.device_model)

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
            "status": "online"
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
        self.model.setInterface("Motor")
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
