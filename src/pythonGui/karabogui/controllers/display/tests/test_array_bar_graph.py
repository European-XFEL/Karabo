from unittest.mock import patch

import numpy as np

from ..array_bar_graph import ArrayBarGraph
from karabo.common.scenemodel.api import VectorBarGraphModel

from karabo.native import Configurable, Hash, Int32, NDArray, VectorInt32
from karabogui.testing import (GuiTestCase, get_class_property_proxy,
                               set_proxy_value)


class Object(Configurable):
    value = VectorInt32()


class NDArrayObject(Configurable):
    value = NDArray(
        defaultValue=np.arange(10),
        shape=(10,),
        dtype=Int32,
    )


class TestDisplayVectorBar(GuiTestCase):
    def setUp(self):
        super(TestDisplayVectorBar, self).setUp()

        schema = Object.getClassSchema()
        self.value = get_class_property_proxy(schema, 'value')
        self.controller = ArrayBarGraph(proxy=self.value)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestDisplayVectorBar, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_bar_graph_basics(self):
        value = [2, 4, 6]
        set_proxy_value(self.value, 'value', value)
        curve = self.controller._plot
        self.assertEqual(list(curve.opts.get('x')), [0, 1, 2])
        self.assertEqual(list(curve.opts.get('height')), value)

    def test_bar_width(self):
        controller = ArrayBarGraph(proxy=self.value,
                                   model=VectorBarGraphModel())
        controller.create(None)
        action = controller.widget.actions()[9]
        self.assertEqual(action.text(), 'Bar Width')

        dsym = 'karabogui.controllers.display.array_bar_graph.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getDouble.return_value = 2.7, True
            action.trigger()
            self.assertEqual(controller.model.bar_width, 2.7)
            self.assertEqual(controller._plot.opts['width'], 2.7)

        controller.destroy()


class TestArrayBar(GuiTestCase):
    def setUp(self):
        super(TestArrayBar, self).setUp()

        schema = NDArrayObject.getClassSchema()
        self.value = get_class_property_proxy(schema, 'value')
        self.controller = ArrayBarGraph(proxy=self.value)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestArrayBar, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_bar_graph_basics(self):
        value = np.array(
            [2, 3, 2, 3, 2, 3, 2, 3, 2, 3],
            dtype=np.int32)
        array_hash = Hash('type', 12,
                          'data', value.tobytes())
        set_proxy_value(self.value, 'value', array_hash)
        curve = self.controller._plot
        np.testing.assert_array_equal(
            list(curve.opts.get('x')), np.arange(10))
        np.testing.assert_array_equal(
            list(curve.opts.get('height')), value)
