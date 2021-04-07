from platform import system
from unittest import mock, skipIf
import warnings

import numpy as np

from karabo.common.scenemodel.api import VectorGraphModel
from karabo.native import Configurable, VectorBool, VectorFloat, VectorInt32
from karabo.native import Hash, NDArray, UInt32
from karabogui.binding.proxy import PropertyProxy
from karabogui.controllers.display.vector_graph import (
    DisplayNDArrayGraph, DisplayVectorGraph)
from karabogui.graph.plots.utils import generate_baseline
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from karabogui.testing import set_proxy_hash


class ArrayObject(Configurable):
    prop = NDArray(
        displayedName="NDArray",
        dtype=UInt32,
        shape=(10,))


class VectorObject(Configurable):
    prop = VectorFloat()
    value = VectorInt32(defaultValue=[1, 2, 3])
    bool_value = VectorBool()


class TestVectorGraph(GuiTestCase):
    def setUp(self):
        super(TestVectorGraph, self).setUp()

        schema = VectorObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, "prop")
        device = self.proxy.root_proxy
        self.value = PropertyProxy(root_proxy=device, path="value")
        self.bool_value = PropertyProxy(root_proxy=device, path="bool_value")
        self.controller = DisplayVectorGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestVectorGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @skipIf(system() == "Windows",
            reason="Crosshair does not toggle off with emit==False")
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
        set_proxy_value(self.proxy, "prop", value)
        self.assertEqual(list(curve.yData), value)

    def test_set_value_inf(self):
        self.assertEqual(len(self.controller._curves), 1)
        curve = self.controller._curves.get(self.proxy)
        self.assertIsNotNone(curve)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, "prop", value)
        self.assertEqual(list(curve.yData), value)

        value = [2, np.inf, 6]
        set_proxy_value(self.proxy, "prop", value)
        self.assertEqual(list(curve.yData), value)

        value = [np.inf, np.inf, np.inf]
        set_proxy_value(self.proxy, "prop", value)

        # None curve in PyQtGraph >= 0.11.1
        self.assertEqual(list(curve.yData), value)

        value = [np.NaN, np.NaN, np.NaN]
        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            set_proxy_value(self.proxy, "prop", value)
            # Note: Older PyQtGraph version <= 0.11.0 are not catching all
            # NaN values.
            np.testing.assert_almost_equal(list(curve.yData), value)

    def test_visualize_prop(self):
        self.controller.visualize_additional_property(self.value)
        self.assertEqual(len(self.controller._curves), 2)
        curve = self.controller._curves.get(self.value)
        self.assertIsNotNone(curve)
        value = [6, 12, 6]
        set_proxy_value(self.value, "value", value)
        self.assertEqual(list(curve.yData), value)

    def test_set_bool_value(self):
        self.controller.visualize_additional_property(self.bool_value)
        self.assertEqual(len(self.controller._curves), 2)
        curve = self.controller._curves.get(self.bool_value)
        self.assertIsNotNone(curve)
        value = [False, False, True]
        set_proxy_value(self.bool_value, "bool_value", value)
        self.assertEqual(list(curve.yData), [0, 0, 1])

    def test_action_names(self):
        controller = DisplayVectorGraph(proxy=self.proxy,
                                        model=VectorGraphModel())
        controller.create(None)
        action = controller.widget.actions()[0]
        self.assertEqual(action.text(), "Grid X")
        action.trigger()
        self.assertEqual(controller.model.x_grid, False)
        action.trigger()
        self.assertEqual(controller.model.x_grid, True)

        action = controller.widget.actions()[1]
        self.assertEqual(action.text(), "Grid Y")
        action.trigger()
        self.assertEqual(controller.model.y_grid, False)
        action.trigger()
        self.assertEqual(controller.model.y_grid, True)

        action = controller.widget.actions()[2]
        self.assertEqual(action.text(), "Log X")
        action.trigger()
        self.assertEqual(controller.model.x_log, True)
        action.trigger()
        self.assertEqual(controller.model.x_log, False)

        action = controller.widget.actions()[3]
        self.assertEqual(action.text(), "Log Y")
        action.trigger()
        self.assertEqual(controller.model.y_log, True)
        action.trigger()
        self.assertEqual(controller.model.y_log, False)

        action = controller.widget.actions()[4]
        self.assertEqual(action.text(), "Invert X")
        action.trigger()
        self.assertEqual(controller.model.x_invert, True)
        action.trigger()
        self.assertEqual(controller.model.x_invert, False)

        action = controller.widget.actions()[5]
        self.assertEqual(action.text(), "Invert Y")
        action.trigger()
        self.assertEqual(controller.model.y_invert, True)
        action.trigger()
        self.assertEqual(controller.model.y_invert, False)
        controller.destroy()

    def test_xtransform(self):
        value = [1, 2, 3]
        set_proxy_value(self.proxy, "prop", value)

        # Get the action
        action = self._get_controller_action("X-Transformation")
        self.assertIsNotNone(action)

        offset = 20
        step = 10
        # Trigger the transformation configuration
        dialog = ("karabogui.controllers.display.vector_graph."
                  "TransformDialog.get")
        content = {"offset": offset, "step": step}
        with mock.patch(dialog, return_value=(content, True)):
            action.trigger()

        # Check if model is not
        for proxy, curve in self.controller._curves.items():
            x_expected = generate_baseline(proxy.value, offset, step)
            np.testing.assert_array_equal(curve.xData, x_expected)

    def test_view_settings(self):
        # Get the action
        action = self._get_controller_action("View")
        self.assertIsNotNone(action)

        # Trigger the transformation configuration
        dialog = ("karabogui.graph.plots.dialogs.view."
                  "GraphViewDialog.get")
        content = {"title": "TestTitle", "background": "white"}
        with mock.patch(dialog, return_value=(content, True)):
            action.trigger()

        config = self.controller.widget.configuration
        self.assertEqual(config["title"], "TestTitle")
        self.assertEqual(config["background"], "white")

    def _get_controller_action(self, text):
        # Get the x-transformation action
        for action in self.controller.widget.actions():
            if action.text() == text:
                return action


class TestArrayGraph(GuiTestCase):
    def setUp(self):
        super(TestArrayGraph, self).setUp()

        schema = ArrayObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, "prop")
        self.controller = DisplayNDArrayGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestArrayGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        curve = self.controller._curves.get(self.proxy)
        self.assertIsNotNone(curve)
        value = np.array(list(range(10)), dtype="uint32")
        array_hash = Hash("type", 14,
                          "data", value.tobytes())
        h = Hash("prop", array_hash)
        set_proxy_hash(self.proxy, h)
        np.testing.assert_array_equal(curve.yData, value)
