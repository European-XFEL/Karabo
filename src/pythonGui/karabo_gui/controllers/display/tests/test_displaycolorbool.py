from unittest.mock import patch

from PyQt4.QtCore import QByteArray
from PyQt4.QtGui import QWidget

from karabo.common.api import DeviceStatus, State
from karabo.common.scenemodel.api import ColorBoolModel
from karabo.middlelayer import Configurable, Bool
from karabo_gui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding
)
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.testing import GuiTestCase
from ..displaycolorbool import DisplayColorBool


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayColorBool(GuiTestCase):
    def setUp(self):
        super(TestDisplayColorBool, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.proxy = PropertyProxy(root_proxy=device, path='prop')
        self.model = ColorBoolModel()

    def test_basics(self):
        controller = DisplayColorBool(proxy=self.proxy, model=self.model)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_exercise_code_paths(self):
        target = 'karabo_gui.controllers.display.displaycolorbool.QSvgWidget'
        with patch(target, new=MockQSvgWidget):
            self.proxy.value = False

            controller = DisplayColorBool(proxy=self.proxy, model=self.model)
            controller.create(None)

            self.proxy.value = True
            active = controller.icon.with_color(STATE_COLORS[State.ACTIVE])
            assert controller.widget.loaded_data == QByteArray(active)

            self.proxy.value = False
            passive = controller.icon.with_color(STATE_COLORS[State.PASSIVE])
            assert controller.widget.loaded_data == QByteArray(passive)
