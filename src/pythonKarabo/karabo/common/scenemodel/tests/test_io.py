# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import os
import os.path as op
from io import StringIO
from xml.etree.ElementTree import Element, fromstring, tostring

from pytest import raises as assert_raises

from karabo.testing.utils import temp_cwd, temp_xml_file, xml_is_equal

# Import via the API module so that all the readers/writers get registered
from .. import api
from ..io_utils import set_numbers

DATA_DIR = op.join(op.abspath(op.dirname(__file__)), "data")
INKSCAPE_DIR = op.join(DATA_DIR, "inkscape")
LEGACY_DIR = op.join(DATA_DIR, "legacy")
SCENE_SVG = """
<svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg"
    height="768"
    krb:version="2"
    krb:uuid="e24a23c7-5aa9-420c-9741-248ea6672355"
    width="1024"
    krb:random="golly" >
    <svg:g
        krb:class="FixedLayout"
        krb:height="323"
        krb:width="384"
        krb:x="106"
        krb:y="74">
        <svg:rect
            height="60"
            width="309"
            x="175" y="125"
            krb:class="Label"
            krb:font="Ubuntu,48,-1,5,63,0,0,0,0,0"
            krb:foreground="#4c4c4c"
            krb:frameWidth="0"
            krb:text="Some text" />
        <svg:rect
            fill="none"
            height="143"
            stroke="#000000"
            stroke-dasharray=""
            stroke-dashoffset="0.0"
            stroke-linecap="square"
            stroke-linejoin="bevel"
            stroke-miterlimit="2.0"
            stroke-opacity="1.0"
            stroke-style="1"
            stroke-width="1.0"
            width="151"
            x="106"
            y="74" />
        <svg:line
            fill="none"
            stroke="#000000"
            stroke-dasharray=""
            stroke-dashoffset="0.0"
            stroke-linecap="square"
            stroke-linejoin="bevel"
            stroke-miterlimit="2.0"
            stroke-opacity="1.0"
            stroke-style="1"
            stroke-width="1.0"
            x1="397"
            x2="489"
            y1="84"
            y2="396" />
    </svg:g>
<metadata id="some-extra-data"><foo>bar</foo></metadata>
</svg>
"""
UNKNOWN_WIDGET_SVG = """
<svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg" >
    <svg:rect
        height="0" width="0" x="0" y="0"
        krb:widget="FutureStyles"
        krb:class="DisplayWidget" />
</svg>
"""

UNKNOWN_WIDGET_TOOL = """
<svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg" >
    <svg:rect
        height="0" width="0" x="0" y="0"
        krb:class="NoWeb" />
</svg>
"""


def _get_file_data(filename):
    with open(filename) as fp:
        return fp.read()


def _iter_data_files(directory):
    for fn in os.listdir(directory):
        if op.splitext(fn)[-1] == ".svg":
            yield op.join(directory, fn)


def test_set_numbers_name_mapping():
    element = Element("rect")
    model = api.RectangleModel(x=0.0, y=0, height=10, width=10)
    names = ("x", "y", "height", "width")
    xmlnames = (api.NS_KARABO + n for n in names[:-1])

    # Normal mapping is tested by the scene writers. Lets make sure the
    # parameter checking works...
    assert_raises(
        api.SceneWriterException,
        set_numbers,
        names,
        model,
        element,
        xmlnames=xmlnames,
    )


def test_set_numbers_float_conversion():
    element = Element("rect")
    model = api.RectangleModel(x=0.0, y=0.2, height=10.01, width=10.05)

    # Test rounding of floating point values
    set_numbers(("x", "y", "height", "width"), model, element)
    assert element.get("x") == "0"
    # Since 2.17.X casted to integers for compatibility
    assert element.get("y") == "0"
    assert element.get("height") == "10"
    assert element.get("width") == "10"


def test_reading():
    with temp_xml_file(SCENE_SVG) as fn:
        scene = api.read_scene(fn)

    assert scene.width == 1024
    assert scene.height == 768
    assert len(scene.children) == 2

    layout = scene.children[0]
    assert len(layout.children) == 3

    child = layout.children[0]
    assert isinstance(child, api.LabelModel)

    child = layout.children[1]
    assert isinstance(child, api.RectangleModel)

    child = layout.children[2]
    assert isinstance(child, api.LineModel)


def test_writing():
    extra_attributes = {api.NS_KARABO + "random": "golly"}
    scene = api.SceneModel(
        extra_attributes=extra_attributes,
        uuid="e24a23c7-5aa9-420c-9741-248ea6672355",
    )
    layout = api.FixedLayoutModel(x=106, y=74, height=323, width=384)
    label = api.LabelModel(
        x=175,
        y=125,
        height=60,
        width=309,
        text="Some text",
        font="Ubuntu,48,-1,5,63,0,0,0,0,0",
        foreground="#4c4c4c",
        frame_width=0,
    )
    rect = api.RectangleModel(
        x=106,
        y=74,
        height=143,
        width=151,
        stroke="#000000",
        stroke_dashoffset=0.0,
        stroke_linecap="square",
        stroke_linejoin="bevel",
        stroke_miterlimit=2.0,
        stroke_opacity=1.0,
        stroke_style=1,
        stroke_width=1.0,
    )
    line = api.LineModel(
        x1=397,
        x2=489,
        y1=84,
        y2=396,
        stroke="#000000",
        stroke_dashoffset=0.0,
        stroke_linecap="square",
        stroke_linejoin="bevel",
        stroke_miterlimit=2.0,
        stroke_opacity=1.0,
        stroke_style=1,
        stroke_width=1.0,
    )
    layout.children.extend([label, rect, line])
    scene.children.append(layout)

    unknown_child = api.UnknownXMLDataModel(tag="foo", data="bar")
    unknown = api.UnknownXMLDataModel(
        tag="metadata",
        attributes={"id": "some-extra-data"},
        children=[unknown_child],
    )
    scene.children.append(unknown)

    xml = api.write_scene(scene)
    assert xml_is_equal(SCENE_SVG, xml)


def test_scene_version():
    scene = api.SceneModel()
    xml = api.write_scene(scene)
    scene = api.read_scene(StringIO(xml))
    assert scene.file_format_version == api.SCENE_FILE_VERSION


def test_single_model_writing():
    expected_svg = """<svg><foo>bar</foo></svg>"""
    model = api.UnknownXMLDataModel(tag="foo", data="bar")
    xml = api.write_single_model(model)
    assert xml_is_equal(expected_svg, xml)


def test_simple_round_trip():
    with temp_xml_file(SCENE_SVG) as fn:
        scene = api.read_scene(fn)

    xml = api.write_scene(scene)
    assert xml_is_equal(SCENE_SVG, xml)


def test_real_data_reading():
    for fn in _iter_data_files(DATA_DIR):
        api.read_scene(fn)


def test_real_data_reading_inkscape():
    for fn in _iter_data_files(INKSCAPE_DIR):
        api.read_scene(fn)


def test_real_data_reading_legacy():
    with temp_cwd(LEGACY_DIR):
        for fn in _iter_data_files(LEGACY_DIR):
            api.read_scene(fn)


def test_real_data_round_trip():
    for fn in _iter_data_files(DATA_DIR):
        scene = api.read_scene(fn)
        new_xml = api.write_scene(scene)
        orig_xml = _get_file_data(fn)

        failmsg = f"Scene {op.basename(fn)} didn't round trip!"
        assert xml_is_equal(orig_xml, new_xml), failmsg


def test_uuid_replacement():
    with temp_xml_file(SCENE_SVG) as fn:
        scene = api.read_scene(fn)

    old_uuid = scene.uuid

    scene.reset_uuid()
    xml = api.write_scene(scene)
    with temp_xml_file(xml) as fn:
        scene = api.read_scene(fn)

    assert old_uuid != scene.uuid


def test_unknown_widget_reader():
    with temp_xml_file(UNKNOWN_WIDGET_SVG) as fn:
        scene = api.read_scene(fn)
    xml = api.write_scene(scene)
    assert xml_is_equal(UNKNOWN_WIDGET_SVG, xml)


def test_unknown_widget_reader_tool():
    """Tool widgets don't have widget trait set"""
    with temp_xml_file(UNKNOWN_WIDGET_TOOL) as fn:
        scene = api.read_scene(fn)
    xml = api.write_scene(scene)
    assert xml_is_equal(UNKNOWN_WIDGET_TOOL, xml)


def test_unknown_widget_writer():
    model = api.UnknownWidgetDataModel(
        klass="FutureStyles", parent_component="DisplayWidget"
    )
    xml = api.write_single_model(model)
    assert xml_is_equal(UNKNOWN_WIDGET_SVG, xml)


def test_unknown_widget_extra_data():
    root = fromstring(UNKNOWN_WIDGET_SVG)
    widget = root[0]
    widget.set("garbage", "value")
    widget.text = "element data"
    expected_svg = tostring(root, encoding="unicode")

    with temp_xml_file(expected_svg) as fn:
        scene = api.read_scene(fn)

    widget = scene.children[0]
    assert widget.attributes["garbage"] == "value"
    assert widget.data == "element data"

    xml = api.write_single_model(widget)
    assert xml_is_equal(expected_svg, xml)


def test_display_label_writer():
    # Check if default values are NOT saved
    default_model = api.DisplayLabelModel(
        font_size=api.SCENE_FONT_SIZE, font_weight=api.SCENE_FONT_WEIGHT
    )
    default_element = _get_xml_element_from_model(default_model)
    assert default_element.get(api.NS_KARABO + "font_size") is None
    assert default_element.get(api.NS_KARABO + "font_weight") is None

    # Check if other values are NOT saved
    input_size = 7
    input_weight = "bold"
    valid_model = api.DisplayLabelModel(
        font_size=input_size, font_weight=input_weight
    )
    valid_element = _get_xml_element_from_model(valid_model)
    assert valid_element.get(api.NS_KARABO + "font_size") == str(input_size)
    assert valid_element.get(api.NS_KARABO + "font_weight") == input_weight


def _get_xml_element_from_model(model):
    # Generate the xml of the model
    xml = api.write_single_model(model)
    # Get the Karabo klass name from the model type
    # (by removing Model from the name)
    name = type(model).__name__.replace("Model", "")

    # Read the generated XML and extract the element
    # corresponding to the Karabo name
    tree = fromstring(xml)
    return tree.find(
        f"*[@{api.NS_KARABO}widget='{name}']"
    )
