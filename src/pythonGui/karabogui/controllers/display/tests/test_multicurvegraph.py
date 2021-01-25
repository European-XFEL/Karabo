from karabo.native import Configurable, Bool, Int32
from karabogui.binding.api import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..multicurvegraph import DisplayMultiCurveGraph


class Object(Configurable):
    reset = Bool()
    index = Int32()
    value = Int32()


class TestMultiCurveGraph(GuiTestCase):
    def setUp(self):
        super(TestMultiCurveGraph, self).setUp()

        schema = Object.getClassSchema()
        self.reset = get_class_property_proxy(schema, 'reset')
        device = self.reset.root_proxy

        self.index = PropertyProxy(root_proxy=device, path='index')
        self.value = PropertyProxy(root_proxy=device, path='value')

        self.controller = DisplayMultiCurveGraph(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.reset)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        super(TestMultiCurveGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self.assertEqual(len(self.controller._curves), 1)
        set_proxy_value(self.index, 'index', 1)
        set_proxy_value(self.value, 'value', 42)
        # The x-proxy does not have a curve
        empty = self.controller._curves.get(self.index)
        self.assertIsNone(empty)
        curve = self.controller._curves.get(self.value)
        self.assertIsNotNone(curve)
        self.assertEqual(list(curve.xData), [1.0])
        self.assertEqual(list(curve.yData), [42.0])
        set_proxy_value(self.index, 'index', 2)
        set_proxy_value(self.value, 'value', 37)
        self.assertEqual(list(curve.xData), [1.0, 2.0])
        self.assertEqual(list(curve.yData), [42.0, 37.0])

        # Reset the curve
        set_proxy_value(self.reset, 'reset', True)
        curve = self.controller._curves.get(self.value)
        self.assertIsNotNone(curve)

        # None curve in PyQtGraph >= 0.11.1
        empty_x = curve.xData is None or list(curve.xData) == []
        empty_y = curve.yData is None or list(curve.yData) == []
        self.assertTrue(empty_x)
        self.assertTrue(empty_y)

        # Slightly refill the curve
        set_proxy_value(self.value, 'value', 42)
        curve = self.controller._curves.get(self.value)

        empty = curve.yData is None or list(curve.yData) == []
        self.assertTrue(empty)
        # No data, we have to list the synchronizer first
        set_proxy_value(self.index, 'index', 1)
        self.assertEqual(list(curve.yData), [42.0])
