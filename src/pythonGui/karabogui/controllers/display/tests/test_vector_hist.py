import numpy as np

from ..display_vector_hist import DisplayHistGraph

from karabo.native import Configurable, VectorFloat
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorFloat(defaultValue=[2.3, 4.5, 7.9])


class TestVectorHistGraph(GuiTestCase):
    def setUp(self):
        super(TestVectorHistGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayHistGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_scatter_graph_basics(self):
        self.controller.model.auto = False
        self.controller.model.start = 0
        self.controller.model.stop = 1
        set_proxy_value(self.proxy, 'prop', [1, 1, 1, 1, 1, 1, 1, 1])
        curve = self.controller._plot
        x, y = curve.getData()
        np.testing.assert_array_almost_equal(
            x, [-0.05, 0.061111, 0.172222, 0.283333, 0.394444, 0.505556,
                0.616667, 0.727778, 0.838889, 0.95])
        np.testing.assert_array_almost_equal(y, [0, 0, 0, 0, 0, 0, 0, 0, 8])
