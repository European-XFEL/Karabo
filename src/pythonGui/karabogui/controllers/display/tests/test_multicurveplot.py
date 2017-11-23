from unittest.mock import patch

from PyQt4.QtGui import QWidget

from karabo.middlelayer import Configurable, Bool, Int32
from karabogui.binding.api import PropertyProxy
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..multicurveplot import MultiCurvePlot


class Object(Configurable):
    reset = Bool()
    index = Int32()
    value = Int32()


class MockMplCurvePlot(QWidget):
    def __init__(self, parent, legend):
        super(MockMplCurvePlot, self).__init__(parent)

    def new_curve(self, *args, **kwargs):
        pass

    def update_curve(self, *args, **kwargs):
        pass

    def axes_call(self, *args, **kwargs):
        pass

    def destroy(self):
        pass


class TestMultiCurvePlot(GuiTestCase):
    def setUp(self):
        super(TestMultiCurvePlot, self).setUp()

        schema = Object.getClassSchema()
        self.reset = get_class_property_proxy(schema, 'reset')
        device = self.reset.root_proxy

        self.index = PropertyProxy(root_proxy=device, path='index')
        self.value = PropertyProxy(root_proxy=device, path='value')

        sym = 'karabogui.controllers.display.multicurveplot.MplCurvePlot'
        self.patcher = patch(sym, new=MockMplCurvePlot)
        self.patcher.start()

        self.controller = MultiCurvePlot(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.reset)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None
        self.patcher.stop()

    def test_set_value(self):
        self.index.value = 1
        self.value.value = 42

    def test_reset(self):
        self.reset.value = True
