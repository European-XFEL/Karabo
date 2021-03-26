from unittest.mock import patch

import numpy as np

from ..display_vector_scatter import DisplayVectorScatterGraph
from karabo.common.scenemodel.api import VectorScatterGraphModel

from karabo.native import Configurable, VectorFloat, VectorInt32
from karabogui.binding.proxy import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    x = VectorFloat(defaultValue=[2.3, 4.5, 7.9])
    y = VectorInt32(defaultValue=[1, 2, 3])


class TestVectorScatterGraph(GuiTestCase):
    def setUp(self):
        super(TestVectorScatterGraph, self).setUp()

        schema = Object.getClassSchema()
        self.x = get_class_property_proxy(schema, 'x')
        device = self.x.root_proxy
        self.y = PropertyProxy(root_proxy=device, path='y')
        self.controller = DisplayVectorScatterGraph(proxy=self.x)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.y)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestVectorScatterGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_scatter_graph_basics(self):
        set_proxy_value(self.x, 'x', [2.3, 4.5, 7.9])
        set_proxy_value(self.y, 'y', [1, 2, 3])
        curve = self.controller._plot
        x, y = curve.getData()
        np.testing.assert_array_almost_equal(x, [2.3, 4.5, 7.9])
        np.testing.assert_array_almost_equal(y, [1, 2, 3])

    def test_pointsize(self):
        controller = DisplayVectorScatterGraph(proxy=self.x,
                                               model=VectorScatterGraphModel())
        controller.create(None)
        action = controller.widget.actions()[10]
        self.assertEqual(action.text(), 'Point Size')

        dsym = ('karabogui.controllers.display.'
                'display_vector_scatter.QInputDialog')
        with patch(dsym) as QInputDialog:
            QInputDialog.getDouble.return_value = 2.7, True
            action.trigger()
            self.assertEqual(controller.model.psize, 2.7)
            self.assertEqual(controller._plot.opts['size'], 2.7)

        controller.destroy()
