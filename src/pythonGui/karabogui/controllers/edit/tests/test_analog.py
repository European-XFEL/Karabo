from karabo.common.scenemodel.api import KnobModel, SliderModel
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..analog import Knob, Slider
from .utils import InRangeInt, LargeRange, Object, Other


class TestEditAnalog(GuiTestCase):
    def setUp(self):
        super(TestEditAnalog, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = Slider(proxy=self.proxy, model=SliderModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_knob(self):
        controller = Knob(proxy=self.proxy, model=KnobModel())
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 1.0)
        assert self.controller.widget.value() == 1.0

    def test_edit_value(self):
        self.controller.widget.valueChanged.emit(3.0)
        assert self.proxy.edit_value == 3.0

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = Slider(proxy=proxy)
        controller.create(None)

        assert controller.widget.minimum() == 1
        assert controller.widget.maximum() == 4

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller.widget.minimum() == -2.0
        assert controller.widget.maximum() == 4.0

    def test_large_range(self):
        proxy = get_class_property_proxy(LargeRange.getClassSchema(), 'prop')
        controller = Slider(proxy=proxy)
        controller.create(None)

        assert not controller.widget.isEnabled()

    def test_large_int(self):
        proxy = get_class_property_proxy(InRangeInt.getClassSchema(), 'prop')
        controller = Slider(proxy=proxy)
        controller.create(None)

        assert controller.widget.isEnabled()
