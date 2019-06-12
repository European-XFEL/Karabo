from unittest.mock import patch

from karabo.common.scenemodel.api import VectorGraphModel
from ..display_vector_graph import DisplayVectorGraph

from karabo.native import Configurable, VectorFloat, VectorInt32
from karabogui.binding.proxy import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorFloat()
    value = VectorInt32(defaultValue=[1, 2, 3])


class TestVectorGraph(GuiTestCase):
    def setUp(self):
        super(TestVectorGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        device = self.proxy.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')
        self.controller = DisplayVectorGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self.assertEqual(len(self.controller._curves), 1)
        curve = self.controller._curves.get(self.proxy)
        self.assertIsNotNone(curve)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, 'prop', value)
        self.assertEqual(list(curve.yData), value)

    def test_visualize_prop(self):
        self.controller.visualize_additional_property(self.value)
        self.assertEqual(len(self.controller._curves), 2)
        curve = self.controller._curves.get(self.value)
        self.assertIsNotNone(curve)
        value = [6, 12, 6]
        set_proxy_value(self.value, 'value', value)
        self.assertEqual(list(curve.yData), value)

    def test_downsample(self):
        controller = DisplayVectorGraph(proxy=self.proxy,
                                        model=VectorGraphModel())
        controller.create(None)
        action = controller.widget.actions()[9]
        self.assertEqual(action.text(), 'Downsample')

        dsym = ('karabogui.controllers.display.'
                'display_vector_graph.QInputDialog')
        with patch(dsym) as QInputDialog:
            QInputDialog.getInt.return_value = 12000, True
            action.trigger()
            self.assertEqual(controller.model.half_samples, 12000)
        controller.destroy()

    def test_action_names(self):
        controller = DisplayVectorGraph(proxy=self.proxy,
                                        model=VectorGraphModel())
        controller.create(None)
        action = controller.widget.actions()[0]
        self.assertEqual(action.text(), 'Grid X')
        action.trigger()
        self.assertEqual(controller.model.x_grid, True)
        action.trigger()
        self.assertEqual(controller.model.x_grid, False)

        action = controller.widget.actions()[1]
        self.assertEqual(action.text(), 'Grid Y')
        action.trigger()
        self.assertEqual(controller.model.y_grid, True)
        action.trigger()
        self.assertEqual(controller.model.y_grid, False)

        action = controller.widget.actions()[2]
        self.assertEqual(action.text(), 'Log X')
        action.trigger()
        self.assertEqual(controller.model.x_log, True)
        action.trigger()
        self.assertEqual(controller.model.x_log, False)

        action = controller.widget.actions()[3]
        self.assertEqual(action.text(), 'Log Y')
        action.trigger()
        self.assertEqual(controller.model.y_log, True)
        action.trigger()
        self.assertEqual(controller.model.y_log, False)

        action = controller.widget.actions()[4]
        self.assertEqual(action.text(), 'Invert X')
        action.trigger()
        self.assertEqual(controller.model.x_invert, True)
        action.trigger()
        self.assertEqual(controller.model.x_invert, False)

        action = controller.widget.actions()[5]
        self.assertEqual(action.text(), 'Invert Y')
        action.trigger()
        self.assertEqual(controller.model.y_invert, True)
        action.trigger()
        self.assertEqual(controller.model.y_invert, False)
        controller.destroy()
