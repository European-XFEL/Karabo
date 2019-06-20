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

    def test_hist_basics(self):
        self.controller.model.auto = False
        self.controller.model.start = 0
        self.controller.model.stop = 10
        set_proxy_value(self.proxy, 'prop', [1, 2, 2, 3, 3, 3, 3, 8])
        curve = self.controller._plot
        x, y = curve.getData()
        np.testing.assert_array_almost_equal(
            x, [-0.454545, 0.545455, 1.545455, 2.545455, 3.545455, 4.545455,
                5.545455, 6.545455, 7.545455, 8.545455, 9.545455])
        np.testing.assert_array_almost_equal(y, [0, 1, 2, 4, 0, 0, 0, 0, 1, 0])

    def test_hist_empty(self):
        self.controller.model.auto = False
        self.controller.model.start = 0
        self.controller.model.stop = 10
        set_proxy_value(self.proxy, 'prop', [])
        curve = self.controller._plot
        x, y = curve.getData()
        self.assertIsNone(x)
        self.assertIsNone(y)
