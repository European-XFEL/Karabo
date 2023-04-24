# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import numpy as np

from karabo.native import Configurable, VectorInt32
from karabogui.binding.api import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..vector_xy_graph import DisplayVectorXYGraph


class Object(Configurable):
    index = VectorInt32()
    value = VectorInt32()


class TestXYVectorGraph(GuiTestCase):
    def setUp(self):
        super(TestXYVectorGraph, self).setUp()

        schema = Object.getClassSchema()
        self.index = get_class_property_proxy(schema, 'index')
        device = self.index.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')

        self.controller = DisplayVectorXYGraph(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        super(TestXYVectorGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        """Test the setting of both x and y values in the vector xy graph"""
        index = [1, 2, 3, 4, 5]
        value = [42] * 5
        set_proxy_value(self.index, 'index', index)
        set_proxy_value(self.value, 'value', value)

        curve = self.controller._curves[self.value]
        np.testing.assert_almost_equal(list(curve.xData), index)
        np.testing.assert_almost_equal(list(curve.yData), value)
