from unittest.mock import patch

from karabo.middlelayer import Configurable, Int32
from karabo_gui.binding.api import PropertyProxy
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
from ..xyplot import XYPlot


class Object(Configurable):
    index = Int32()
    value = Int32()


class TestXYPlot(GuiTestCase):
    def setUp(self):
        super(TestXYPlot, self).setUp()

        schema = Object.getClassSchema()
        self.index = get_class_property_proxy(schema, 'index')
        device = self.index.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')

        # XXX: Matplotlib cause CI seg fault during gc, at the moment we patch
        # its widget with a mock. (On local machine Ubuntu 16.04, no seg fault)
        sym = "karabo_gui.controllers.display.xyplot.MplCurvePlot"
        self.patcher = patch(sym, spec=True)
        self.patcher.start()

        self.controller = XYPlot(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        self.controller.destroy()
        self.patcher.stop()
        assert self.controller.widget is None

    def test_set_value(self):
        self.index.value = 1
        self.value.value = 42
