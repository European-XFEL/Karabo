from unittest.mock import patch

import numpy as np

from karabo.common.scenemodel.api import VectorGraphModel
from ..display_vector_graph import DisplayVectorGraph

from karabo.native import Configurable, VectorBool, VectorFloat, VectorInt32
from karabogui.binding.proxy import PropertyProxy
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorFloat()
    value = VectorInt32(defaultValue=[1, 2, 3])
    bool_value = VectorBool()


class TestVectorGraph(GuiTestCase):
    def setUp(self):
        super(TestVectorGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        device = self.proxy.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')
        self.bool_value = PropertyProxy(root_proxy=device, path='bool_value')
        self.controller = DisplayVectorGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestVectorGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_crosshair(self):
        cross_target = self.controller.widget._cross_target

        def assert_crosshair_exists(horizontal, vertical):
            self.assertEqual(cross_target.h_line is not None, horizontal)
            self.assertEqual(cross_target.v_line is not None, vertical)

        assert_crosshair_exists(False, False)
        cross_target.action_button.clicked.emit(True)
        assert_crosshair_exists(True, True)
        cross_target.action_button.clicked.emit(False)
        assert_crosshair_exists(False, False)

    def test_set_value(self):
        self.assertEqual(len(self.controller._curves), 1)
        curve = self.controller._curves.get(self.proxy)
        self.assertIsNotNone(curve)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, 'prop', value)
        self.assertEqual(list(curve.yData), value)

    def test_set_value_inf(self):
        self.assertEqual(len(self.controller._curves), 1)
        curve = self.controller._curves.get(self.proxy)
        self.assertIsNotNone(curve)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, 'prop', value)
        self.assertEqual(list(curve.yData), value)

        value = [2, np.inf, 6]
        set_proxy_value(self.proxy, 'prop', value)
        self.assertEqual(list(curve.yData), value)

        value = [np.inf, np.inf, np.inf]
        set_proxy_value(self.proxy, 'prop', value)
        self.assertIsNone(curve.yData)

        value = [np.NaN, np.NaN, np.NaN]
        set_proxy_value(self.proxy, 'prop', value)
        np.testing.assert_almost_equal(list(curve.yData), value)

    def test_visualize_prop(self):
        self.controller.visualize_additional_property(self.value)
        self.assertEqual(len(self.controller._curves), 2)
        curve = self.controller._curves.get(self.value)
        self.assertIsNotNone(curve)
        value = [6, 12, 6]
        set_proxy_value(self.value, 'value', value)
        self.assertEqual(list(curve.yData), value)

    def test_set_bool_value(self):
        self.controller.visualize_additional_property(self.bool_value)
        self.assertEqual(len(self.controller._curves), 2)
        curve = self.controller._curves.get(self.bool_value)
        self.assertIsNotNone(curve)
        value = [False, False, True]
        set_proxy_value(self.bool_value, 'bool_value', value)
        self.assertEqual(list(curve.yData), [0, 0, 1])

    def test_downsample(self):
        controller = DisplayVectorGraph(proxy=self.proxy,
                                        model=VectorGraphModel())
        controller.create(None)
        action = controller.widget.actions()[10]
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
        self.assertEqual(controller.model.x_grid, False)
        action.trigger()
        self.assertEqual(controller.model.x_grid, True)

        action = controller.widget.actions()[1]
        self.assertEqual(action.text(), 'Grid Y')
        action.trigger()
        self.assertEqual(controller.model.y_grid, False)
        action.trigger()
        self.assertEqual(controller.model.y_grid, True)

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
