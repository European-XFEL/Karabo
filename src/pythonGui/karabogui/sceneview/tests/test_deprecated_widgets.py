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

import numpy as np
import pytest
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
from karabogui.testing import set_proxy_value

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


@pytest.fixture
def setup(gui_app):
    # Initialize device proxies
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceProxy(binding=binding,
                         server_id='Fake',
                         device_id=DEVICE_NAME,
                         status=ProxyStatus.OFFLINE)

    # Initialize device proxies and create a map for the mock
    proxy_names = VECTOR_PROXIES + NUMBER_PROXIES + IMAGE_PROXIES
    proxy_map = {name: PropertyProxy(root_proxy=device, path=name)
                 for name in proxy_names}

    # Initialize scene view
    view = SceneView()

    # Yield objects and destroy view model in teardown
    yield proxy_map, view
    view.destroy()


def test_qwt_displayplot(setup, mocker):
    proxy_map, view = setup
    # Check if old model from SVG is replaced
    svg = _generate_svg("DisplayPlot", keys=VECTOR_PROXIES)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, VectorGraphModel)

    # Check if number of data items corresponds with the keys
    widget = _get_controller(model, view).widget
    data_items = widget.plotItem.dataItems
    assert len(data_items) == 2

    # Verify data item values. Set values first to trigger data plotting.
    _set_values(proxy_map)
    for curve in data_items:
        name = _get_name(curve.name())
        expected_value = _get_value(name)
        assert name in VECTOR_PROXIES
        assert_array_equal(curve.xData, np.arange(len(expected_value)))
        assert_array_equal(curve.yData, expected_value)


def test_qwt_xyvector(setup, mocker):
    proxy_map, view = setup
    # Check if old model from SVG is replaced
    svg = _generate_svg("XYVector", keys=VECTOR_PROXIES)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, VectorXYGraphModel)

    # Check if number of data items corresponds with the keys
    widget = _get_controller(model, view).widget
    data_items = widget.plotItem.dataItems
    assert len(data_items) == 1

    # Verify data item values. Set values first to trigger data plotting.
    _set_values(proxy_map)
    curve = data_items[0]
    assert_array_equal(curve.xData, _get_value(VECTOR_PROXIES[0]))
    assert_array_equal(curve.yData, _get_value(VECTOR_PROXIES[1]))


def test_qwt_displaytrendline(setup, mocker):
    proxy_map, view = setup
    # Check if old model from SVG is replaced
    svg = _generate_svg("DisplayTrendline", keys=NUMBER_PROXIES)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, TrendGraphModel)

    # Check if number of data items corresponds with the keys
    widget = _get_controller(model, view)._plot
    data_items = widget.plotItem.dataItems
    assert len(data_items) == len(NUMBER_PROXIES)

    # Verify data item values. Set values first to trigger data plotting.
    _set_values(proxy_map)
    for curve in data_items:
        name = _get_name(curve.name())
        expected_value = _get_value(name)
        assert name in NUMBER_PROXIES
        assert_array_equal(curve.yData, [expected_value])


def test_qwt_displayimage(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("DisplayImage", keys=[IMAGE])
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, ImageGraphModel)
    _assert_image_model(model, proxy_map, view)


def test_qwt_displayalignedimage(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("DisplayAlignedImage", keys=[IMAGE])
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, DetectorGraphModel)
    _assert_image_model(model, proxy_map, view)


def test_qwt_displayimageelement(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("DisplayImageElement", keys=[IMAGE])
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, WebCamGraphModel)
    _assert_image_model(model, proxy_map, view)


def test_qwt_webcamimage(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("WebcamImage", keys=[IMAGE], image_attrs=True)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, WebCamGraphModel)
    _assert_image_model(model, proxy_map, view)


def test_qwt_scientificimage(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("ScientificImage", keys=[IMAGE], image_attrs=True)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, WebCamGraphModel)
    _assert_image_model(model, proxy_map, view)


@pytest.mark.skipif(system() == "Windows",
                    reason="curve.data is empty on Windows")
def test_mpl_xyplot(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("XYPlot", keys=NUMBER_PROXIES[:2])
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, ScatterGraphModel)

    # Verify scatter item values.
    # Set values first to trigger data plotting.
    _set_values(proxy_map)
    curve = _get_controller(model, view)._plot
    x_value = _get_value(NUMBER_PROXIES[0])
    y_value = _get_value(NUMBER_PROXIES[1])
    assert_array_equal(curve.data['x'], [x_value])
    assert_array_equal(curve.data['y'], [y_value])


def test_mpl_multicurveplot(setup, mocker):
    proxy_map, view = setup
    svg = _generate_svg("MultiCurvePlot", keys=NUMBER_PROXIES)
    model = _load_scene(svg, proxy_map, view, mocker)
    assert isinstance(model, MultiCurveGraphModel)

    # Check if number of data items corresponds with the keys
    widget = _get_controller(model, view).widget
    data_items = widget.plotItem.dataItems
    assert len(data_items) == len(NUMBER_PROXIES) - 1

    # Verify data item values. Set values first to trigger data plotting.
    _set_values(proxy_map)
    for curve in data_items:
        name = _get_name(curve.name())
        assert name in NUMBER_PROXIES
        assert_array_equal(curve.xData, _get_value(X_NUMBER))
        assert_array_equal(curve.yData, _get_value(name))


# -----------------------------------------------------------------------
# Helpers

def _assert_image_model(model, proxy_map, view):
    # Verify data item values. Set values first to trigger data plotting.
    _set_image(proxy_map)
    widget = _get_controller(model, view).widget
    image = widget.plotItem.imageItem.image
    assert_array_equal(image, np.zeros((dimY, dimX)))


def _load_scene(svg, proxy_map, view, mocker):
    def _get_proxy(_, name):
        """Return a `PropertyProxy` instance for a given device and property
        path. This mocks the get_proxy called by ControllerContainer."""
        return proxy_map.get(name)

    with StringIO(svg) as fp:
        scene_model = read_scene(fp)

    mocker.patch(GET_PROXY_PATH, new=_get_proxy)
    view.update_model(scene_model)

    return scene_model.children[0]


def _get_controller(model, view):
    container = view._scene_obj_cache.get(model)
    return container.widget_controller


def _set_values(proxy_map):
    """Set numerical values"""
    for name in VECTOR_PROXIES + NUMBER_PROXIES:
        proxy = proxy_map.get(name)
        set_proxy_value(proxy, name, _get_value(name))


def _set_image(proxy_map, dimZ=None):
    output_proxy = proxy_map.get(OUTPUT)
    apply_configuration(get_image_hash(dimZ=dimZ), output_proxy.binding)


def _get_path(name):
    return ".".join([DEVICE_NAME, name])


def _get_paths(names):
    return [_get_path(name) for name in names]


def _get_name(path):
    return path.split(".")[-1]


def _get_value(name):
    return VALUES[name]


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
    paths = _get_paths(keys)
    return svg.format(widget=widget, keys=",".join(paths), attrs=attrs)
