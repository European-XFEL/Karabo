# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from io import StringIO
from platform import system
from unittest import mock, skipIf

import numpy as np
from numpy.testing import assert_array_equal

from karabo.common.scenemodel.api import (
    DetectorGraphModel, ImageGraphModel, MultiCurveGraphModel,
    ScatterGraphModel, TrendGraphModel, VectorGraphModel, VectorXYGraphModel,
    WebCamGraphModel, read_scene)
from karabo.native import (
    Configurable, Float, Int32, Node, UInt32, VectorFloat, VectorInt32)
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, ProxyStatus, apply_configuration,
    build_binding)
from karabogui.controllers.display.tests.image import (
    dimX, dimY, get_image_hash, get_output_node)
from karabogui.testing import GuiTestCase, set_proxy_value

from ..api import SceneView


class Object(Configurable):
    x_vector = VectorFloat()
    y_vector = VectorInt32()

    x_number = Float()
    y_number = Int32()
    z_number = UInt32()

    output = Node(get_output_node(), displayType='OutputChannel')


DEVICE_NAME = "DEVICE"
X_VECTOR = "x_vector"
Y_VECTOR = "y_vector"
X_NUMBER = "x_number"
Y_NUMBER = "y_number"
Z_NUMBER = "z_number"
OUTPUT = "output.data"
IMAGE = "output.data.image"

VECTOR_PROXIES = [X_VECTOR, Y_VECTOR]
NUMBER_PROXIES = [X_NUMBER, Y_NUMBER, Z_NUMBER]
IMAGE_PROXIES = [OUTPUT, IMAGE]

# Create a map for setting values
VALUES = {
    X_VECTOR: [1, 2, 3],
    Y_VECTOR: [1, 4, 9],
    X_NUMBER: 1,
    Y_NUMBER: 2,
    Z_NUMBER: 3}

GET_PROXY_PATH = "karabogui.sceneview.widget.container.get_proxy"


class TestDeprecatedWidgets(GuiTestCase):

    # -----------------------------------------------------------------------
    # Test setup

    def setUp(self):
        super().setUp()

        # Initialize device proxies
        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceProxy(binding=binding,
                             server_id='Fake',
                             device_id=DEVICE_NAME,
                             status=ProxyStatus.OFFLINE)

        # Initialize device proxies and create a map for the mock
        proxy_names = VECTOR_PROXIES + NUMBER_PROXIES + IMAGE_PROXIES
        self.proxy_map = {name: PropertyProxy(root_proxy=device, path=name)
                          for name in proxy_names}

        # Initialize scene view
        self.view = SceneView()

    def tearDown(self):
        super().tearDown()
        self.view.destroy()
        self.view = None

    # -----------------------------------------------------------------------
    # Actual tests

    def test_qwt_displayplot(self):
        # Check if old model from SVG is replaced
        svg = self._generate_svg("DisplayPlot", keys=VECTOR_PROXIES)
        model = self._load_scene(svg)
        self.assertIsInstance(model, VectorGraphModel)

        # Check if number of data items corresponds with the keys
        widget = self._get_controller(model).widget
        data_items = widget.plotItem.dataItems
        self.assertEqual(len(data_items), 2)

        # Verify data item values. Set values first to trigger data plotting.
        self._set_values()
        for curve in data_items:
            name = self._get_name(curve.name())
            expected_value = self._get_value(name)
            self.assertTrue(name in VECTOR_PROXIES)
            assert_array_equal(curve.xData, np.arange(len(expected_value)))
            assert_array_equal(curve.yData, expected_value)

    def test_qwt_xyvector(self):
        # Check if old model from SVG is replaced
        svg = self._generate_svg("XYVector", keys=VECTOR_PROXIES)
        model = self._load_scene(svg)
        self.assertIsInstance(model, VectorXYGraphModel)

        # Check if number of data items corresponds with the keys
        widget = self._get_controller(model).widget
        data_items = widget.plotItem.dataItems
        self.assertEqual(len(data_items), 1)

        # Verify data item values. Set values first to trigger data plotting.
        self._set_values()
        curve = data_items[0]
        assert_array_equal(curve.xData, self._get_value(VECTOR_PROXIES[0]))
        assert_array_equal(curve.yData, self._get_value(VECTOR_PROXIES[1]))

    def test_qwt_displaytrendline(self):
        # Check if old model from SVG is replaced
        svg = self._generate_svg("DisplayTrendline", keys=NUMBER_PROXIES)
        model = self._load_scene(svg)
        self.assertIsInstance(model, TrendGraphModel)

        # Check if number of data items corresponds with the keys
        widget = self._get_controller(model)._plot
        data_items = widget.plotItem.dataItems
        self.assertEqual(len(data_items), len(NUMBER_PROXIES))

        # Verify data item values. Set values first to trigger data plotting.
        self._set_values()
        for curve in data_items:
            name = self._get_name(curve.name())
            expected_value = self._get_value(name)
            self.assertTrue(name in NUMBER_PROXIES)
            assert_array_equal(curve.yData, [expected_value])

    def test_qwt_displayimage(self):
        svg = self._generate_svg("DisplayImage", keys=[IMAGE])
        model = self._load_scene(svg)
        self.assertIsInstance(model, ImageGraphModel)
        self._assert_image_model(model)

    def test_qwt_displayalignedimage(self):
        svg = self._generate_svg("DisplayAlignedImage", keys=[IMAGE])
        model = self._load_scene(svg)
        self.assertIsInstance(model, DetectorGraphModel)
        self._assert_image_model(model)

    def test_qwt_displayimageelement(self):
        svg = self._generate_svg("DisplayImageElement", keys=[IMAGE])
        model = self._load_scene(svg)
        self.assertIsInstance(model, WebCamGraphModel)
        self._assert_image_model(model)

    def test_qwt_webcamimage(self):
        svg = self._generate_svg("WebcamImage", keys=[IMAGE], image_attrs=True)
        model = self._load_scene(svg)
        self.assertIsInstance(model, WebCamGraphModel)
        self._assert_image_model(model)

    def test_qwt_scientificimage(self):
        svg = self._generate_svg("ScientificImage",
                                 keys=[IMAGE],
                                 image_attrs=True)
        model = self._load_scene(svg)
        self.assertIsInstance(model, WebCamGraphModel)
        self._assert_image_model(model)

    @skipIf(system() == "Windows",
            reason="curve.data is empty on Windows")
    def test_mpl_xyplot(self):
        svg = self._generate_svg("XYPlot", keys=NUMBER_PROXIES[:2])
        model = self._load_scene(svg)
        self.assertIsInstance(model, ScatterGraphModel)

        # Verify scatter item values.
        # Set values first to trigger data plotting.
        self._set_values()
        curve = self._get_controller(model)._plot
        x_value = self._get_value(NUMBER_PROXIES[0])
        y_value = self._get_value(NUMBER_PROXIES[1])
        assert_array_equal(curve.data['x'], [x_value])
        assert_array_equal(curve.data['y'], [y_value])

    def test_mpl_multicurveplot(self):
        svg = self._generate_svg("MultiCurvePlot", keys=NUMBER_PROXIES)
        model = self._load_scene(svg)
        self.assertIsInstance(model, MultiCurveGraphModel)

        # Check if number of data items corresponds with the keys
        widget = self._get_controller(model).widget
        data_items = widget.plotItem.dataItems
        self.assertEqual(len(data_items), len(NUMBER_PROXIES) - 1)

        # Verify data item values. Set values first to trigger data plotting.
        self._set_values()
        for curve in data_items:
            name = self._get_name(curve.name())
            self.assertTrue(name in NUMBER_PROXIES)
            assert_array_equal(curve.xData, self._get_value(X_NUMBER))
            assert_array_equal(curve.yData, self._get_value(name))

    # -----------------------------------------------------------------------
    # Helpers

    def _assert_image_model(self, model):
        # Verify data item values. Set values first to trigger data plotting.
        self._set_image()
        widget = self._get_controller(model).widget
        image = widget.plotItem.imageItem.image
        assert_array_equal(image, np.zeros((dimY, dimX)))

    def _load_scene(self, svg):
        with StringIO(svg) as fp:
            scene_model = read_scene(fp)

        with mock.patch(GET_PROXY_PATH, new=self._get_proxy):
            self.view.update_model(scene_model)

        return scene_model.children[0]

    def _get_controller(self, model):
        container = self.view._scene_obj_cache.get(model)
        return container.widget_controller

    def _get_proxy(self, _, name):
        """Return a `PropertyProxy` instance for a given device and property
        path. This mocks the get_proxy called by ControllerContainer."""
        return self.proxy_map.get(name)

    def _set_values(self):
        """Set numerical values"""
        for name in VECTOR_PROXIES + NUMBER_PROXIES:
            proxy = self.proxy_map.get(name)
            set_proxy_value(proxy, name, self._get_value(name))

    def _set_image(self, dimZ=None):
        output_proxy = self.proxy_map.get(OUTPUT)
        apply_configuration(get_image_hash(dimZ=dimZ), output_proxy.binding)

    @staticmethod
    def _get_path(name):
        return ".".join([DEVICE_NAME, name])

    @staticmethod
    def _get_paths(names):
        return [TestDeprecatedWidgets._get_path(name) for name in names]

    @staticmethod
    def _get_name(path):
        return path.split(".")[-1]

    @staticmethod
    def _get_value(name):
        return VALUES[name]

    @staticmethod
    def _generate_svg(widget, keys=VECTOR_PROXIES, image_attrs=False):
        attrs = ''
        if image_attrs:
            attrs = """
                krb:show_axes="true"
                krb:show_color_bar="true"
                krb:show_tool_bar="true"
            """
        svg = """
        <svg:svg
            xmlns:krb="http://karabo.eu/scene"
            xmlns:svg="http://www.w3.org/2000/svg"
            height="436" width="484"
            krb:version="2">
            <svg:rect
                height="364" width="419" x="30" y="30"
                krb:class="DisplayComponent"
                krb:keys="{keys}"
                {attrs}
                krb:widget="{widget}"/>
        </svg:svg>
        """
        paths = TestDeprecatedWidgets._get_paths(keys)
        return svg.format(widget=widget, keys=",".join(paths), attrs=attrs)
