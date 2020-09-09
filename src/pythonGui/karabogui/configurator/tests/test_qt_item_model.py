from PyQt5.QtCore import Qt

from karabo.common.api import ProxyStatus, State
from karabo.native import (
    Bool, Configurable, Float, Int32, Slot, String, VectorFloat, VectorHash
)
from karabogui.binding.api import ProjectDeviceProxy, build_binding
from karabogui.testing import GuiTestCase
from ..qt_item_model import ConfigurationTreeModel


class RowSchema(Configurable):
    start = Float()
    stop = Float()


class Object(Configurable):
    state = String(enum=State)

    foo = Bool(defaultValue=True)
    bar = Float(minInc=0, maxInc=10)
    baz = Int32()
    qux = String(options=['foo', 'bar', 'baz', 'qux'])
    vector = VectorFloat(defaultValue=["1"],
                         minSize=1, maxSize=2)
    table = VectorHash(rows=RowSchema)

    @Slot(allowedStates=[State.INTERLOCKED, State.ACTIVE])
    def slot(self):
        pass


class TestConfiguratorModel(GuiTestCase):
    def setUp(self):
        super(TestConfiguratorModel, self).setUp()

        binding = build_binding(Object.getClassSchema())
        root = ProjectDeviceProxy(binding=binding, server_id='Test',
                                  status=ProxyStatus.OFFLINE)
        self.model = ConfigurationTreeModel()
        self.model.root = root

    def test_basics(self):
        # This is constant
        assert self.model.columnCount() == 3
        # The number of properties in the Object class up above
        assert self.model.rowCount() == 8

    def test_attributes(self):
        bar_index = self.model.index(2, 0)
        assert bar_index.isValid()
        assert bar_index.data() == 'bar'

        # minInc, maxInc and daqPolicy attributes

        # minInc
        assert self.model.rowCount(parent=bar_index) == 3
        min_inc_index = bar_index.child(0, 0)
        assert min_inc_index.data() == 'minInc'

        # maxInc
        max_inc_index = bar_index.child(1, 1)
        assert max_inc_index.data(role=Qt.EditRole) == 10

        # daqPolicy
        daq_index = bar_index.child(2, 2)
        assert daq_index.data() == '-1'

        max_inc_index = bar_index.child(1, 2)
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        assert max_inc_index.flags() == flags

        # ------------------------------------
        # Test the vector
        vector_index = self.model.index(5, 0)
        assert vector_index.isValid()
        assert vector_index.data() == 'vector'
        assert self.model.rowCount(parent=bar_index) == 3

        # daqPolicy
        min_size_index = vector_index.child(0, 0)
        assert min_size_index.data() == 'daqPolicy'

    def test_flags(self):
        bar_index = self.model.index(2, 0)
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled
        assert bar_index.flags() == flags

    def test_parent(self):
        bar_index = self.model.index(2, 0)
        assert not bar_index.parent().isValid()

        min_inc_index = bar_index.child(0, 0)
        assert min_inc_index.parent() == bar_index
