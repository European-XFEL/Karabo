import numpy as np

from karabo.native import Configurable, VectorInt32
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..vector_fill_graph import DisplayVectorFillGraph


class Object(Configurable):
    value = VectorInt32()


class TestDisplayVectorFill(GuiTestCase):
    def setUp(self):
        super().setUp()

        schema = Object.getClassSchema()
        self.value = get_class_property_proxy(schema, 'value')
        self.controller = DisplayVectorFillGraph(proxy=self.value)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_vector_fill_basics(self):
        value = [2, 4, 6]
        set_proxy_value(self.value, 'value', value)
        curve = self.controller._plot
        self.assertIsNotNone(curve)
        np.testing.assert_array_equal(curve.yData, value)
