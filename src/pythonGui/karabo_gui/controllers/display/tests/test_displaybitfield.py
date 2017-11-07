from PyQt4.QtCore import QEvent, QPoint, QRect, Qt
from PyQt4.QtGui import QMouseEvent

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Configurable, Int16, Int32
from karabo_gui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding
)
from karabo_gui.testing import GuiTestCase
from ..displaybitfield import Bitfield


class Object(Configurable):
    prop = Int32(defaultValue=0)


class Object2(Configurable):
    prop = Int16()


class TestDisplayCheckBox(GuiTestCase):
    def setUp(self):
        super(TestDisplayCheckBox, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.proxy = PropertyProxy(root_proxy=device, path='prop')

    def test_basics(self):
        controller = Bitfield(proxy=self.proxy)
        controller.create(None)
        assert controller._internal_widget.size == 32

        controller.destroy()
        assert controller.widget is None

    def test_binding_change(self):
        controller = Bitfield(proxy=self.proxy)
        controller.create(None)

        try:
            schema = Object2.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)
            assert controller._internal_widget.size == 16
        finally:
            schema = Object.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)

    def test_set_value(self):
        controller = Bitfield(proxy=self.proxy)
        controller.create(None)

        self.proxy.value = 42
        assert controller._internal_widget.value == 42

    def test_non_read_only(self):
        self.proxy.value = 0
        controller = Bitfield(proxy=self.proxy)
        controller.create(None)
        bitfield = controller._internal_widget

        # The widget needs its geometry initialized!
        size = bitfield.sizeHint()
        bitfield.setGeometry(QRect(0, 0, size.width(), size.height()))

        # Widget size (W, H): (32 * 5 // 4 = 40, 20)
        # Widget bit size (bW, bH): (W * 4 // 32 = 5, H // 4 = 5)
        # Pos -> bit index: Y // bH + 4 * (X // bW)
        # Bit 4 center: (8, 3)
        mouse_event = QMouseEvent(QEvent.MouseButtonPress,
                                  QPoint(8, 3),
                                  Qt.LeftButton, Qt.LeftButton, Qt.NoModifier)

        # Try to set a bit in read-only mode
        controller.set_read_only(True)
        assert bitfield.focusPolicy() == Qt.NoFocus

        bitfield.mousePressEvent(mouse_event)
        assert bitfield.value == 0

        # Try again in writable mode
        controller.set_read_only(False)
        assert bitfield.focusPolicy() == Qt.StrongFocus

        bitfield.mousePressEvent(mouse_event)
        assert bitfield.value == 1 << 4
