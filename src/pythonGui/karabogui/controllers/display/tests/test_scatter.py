from platform import system
from unittest import skipIf
from unittest.mock import patch

from karabo.common.scenemodel.api import ScatterGraphModel
from karabo.native import Configurable, Double, Hash, Timestamp
from karabogui.binding.proxy import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_hash, set_proxy_value)

from ..scatter_graph import DisplayScatterGraph


class Object(Configurable):
    x = Double()
    y = Double()


class TestScatterGraph(GuiTestCase):
    def setUp(self):
        super(TestScatterGraph, self).setUp()

        schema = Object.getClassSchema()
        self.x = get_class_property_proxy(schema, 'x')
        device = self.x.root_proxy
        self.y = PropertyProxy(root_proxy=device, path='y')
        self.controller = DisplayScatterGraph(proxy=self.x)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.y)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestScatterGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @skipIf(system() == "Windows",
            reason="curve.getData returns empty arrays in Windows")
    def test_scatter_graph_basics(self):
        set_proxy_value(self.x, 'x', 2.1)
        set_proxy_value(self.y, 'y', 3.2)
        curve = self.controller._plot
        self.assertEqual(list(curve.getData()), [2.1, 3.2])

    @skipIf(system() == "Windows",
            reason="curve.getData returns empty arrays in Windows")
    def test_scatter_timestamp(self):
        timestamp = Timestamp()
        set_proxy_value(self.x, 'x', 2.1)
        h = Hash('y', 3.2)
        set_proxy_hash(self.y, h, timestamp)
        curve = self.controller._plot
        self.assertEqual(list(curve.getData()), [2.1, 3.2])
        h = Hash('y', 13.2)
        set_proxy_hash(self.y, h, timestamp)
        self.assertEqual(list(curve.getData()), [2.1, 3.2])

    def test_deque(self):
        controller = DisplayScatterGraph(proxy=self.x,
                                         model=ScatterGraphModel())
        controller.create(None)
        action = controller.widget.actions()[10]
        self.assertEqual(action.text(), 'Queue Size')

        dsym = 'karabogui.controllers.display.scatter_graph.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getInt.return_value = 20, True
            action.trigger()
            self.assertEqual(controller.model.maxlen, 20)
            self.assertEqual(controller._x_values.maxlen, 20)
            self.assertEqual(controller._y_values.maxlen, 20)
        controller.destroy()

    def test_pointsize(self):
        controller = DisplayScatterGraph(proxy=self.x,
                                         model=ScatterGraphModel())
        controller.create(None)
        action = controller.widget.actions()[11]
        self.assertEqual(action.text(), 'Point Size')

        dsym = 'karabogui.controllers.display.scatter_graph.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getDouble.return_value = 2.7, True
            action.trigger()
            self.assertEqual(controller.model.psize, 2.7)
            self.assertEqual(controller._plot.opts['size'], 2.7)
        controller.destroy()
