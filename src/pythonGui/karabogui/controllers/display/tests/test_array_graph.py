import numpy as np

from karabo.native import Configurable, Hash, NDArray, UInt32
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_hash)

from ..display_array_graph import DisplayNDArrayGraph


class Object(Configurable):
    prop = NDArray(
        displayedName="NDArray",
        dtype=UInt32,
        shape=(10,))


class TestArrayGraph(GuiTestCase):
    def setUp(self):
        super(TestArrayGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayNDArrayGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestArrayGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        curve = self.controller._plot
        self.assertIsNotNone(curve)
        value = np.array(list(range(10)), dtype="uint32")
        array_hash = Hash('type', 14,
                          'data', value.tobytes())
        h = Hash('prop', array_hash)
        set_proxy_hash(self.proxy, h)
        np.testing.assert_array_equal(curve.yData, value)
