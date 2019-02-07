from unittest.mock import patch

from PyQt4.QtCore import QByteArray
from PyQt4.QtGui import QWidget

from karabo.common.api import State
from karabo.common.scenemodel.api import ColorBoolModel
from karabo.native import Configurable, Bool, Hash
from karabogui.binding.api import apply_configuration
from karabogui.indicators import STATE_COLORS
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..colorbool import DisplayColorBool


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayColorBool(GuiTestCase):
    def setUp(self):
        super(TestDisplayColorBool, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.model = ColorBoolModel()

    def test_basics(self):
        controller = DisplayColorBool(proxy=self.proxy, model=self.model)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_exercise_code_paths(self):
        target = 'karabogui.controllers.display.colorbool.QSvgWidget'
        with patch(target, new=MockQSvgWidget):
            self.proxy.value = False

            controller = DisplayColorBool(proxy=self.proxy, model=self.model)
            controller.create(None)

            apply_configuration(Hash('prop', True),
                                self.proxy.root_proxy.binding)
            active = controller.icon.with_color(STATE_COLORS[State.ACTIVE])
            assert controller.widget.loaded_data == QByteArray(active)

            apply_configuration(Hash('prop', False),
                                self.proxy.root_proxy.binding)
            passive = controller.icon.with_color(STATE_COLORS[State.PASSIVE])
            assert controller.widget.loaded_data == QByteArray(passive)
