from PyQt5.QtCore import Qt

from karabo.common.scenemodel.api import DisplayProgressBarModel
from karabo.native import Configurable, Float, Int8
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..progressbar import DisplayProgressBar, NULL_RANGE, PROGRESS_MAX


class Object(Configurable):
    prop = Float(minInc=-2.0, maxInc=4.0)


class ObjectWithoutLimits(Configurable):
    prop = Int8()


class TestDisplayProgressBar(GuiTestCase):
    def setUp(self):
        super(TestDisplayProgressBar, self).setUp()

        schema = Object.getClassSchema()
        self.model = DisplayProgressBarModel(is_vertical=True)
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayProgressBar(proxy=self.proxy,
                                             model=self.model)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayProgressBar, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_widget(self):
        controller = DisplayProgressBar(proxy=self.proxy, model=self.model)
        controller.create(None)
        assert controller.widget.minimum() == 0
        assert controller.widget.maximum() == PROGRESS_MAX

    def test_orientation(self):
        assert self.controller.widget.orientation() == Qt.Vertical

        # Trigger the orientation changing action
        action = self.controller.widget.actions()[0]
        assert action.text() == 'Change Orientation'

        action.trigger()
        assert self.controller.widget.orientation() == Qt.Horizontal

    def test_set_value(self):
        # value range is [-2, 4]; 1.0 is the middle
        set_proxy_value(self.proxy, 'prop', 1.0)
        assert self.controller.widget.value() == PROGRESS_MAX * 0.5

    def test_no_limits_messagebox(self):
        schema = ObjectWithoutLimits.getClassSchema()
        build_binding(schema, existing=self.proxy.root_proxy.binding)

        assert self.controller.widget.minimum() == 0
        assert self.controller.widget.maximum() == 0
        assert self.controller._value_factors == NULL_RANGE
