from xml.etree.ElementTree import Element
import os
import os.path as op

from nose.tools import assert_raises

# Import via the API module so that all the readers/writers get registered
from ..api import (SceneModel, FixedLayoutModel, LabelModel, LineModel,
                   RectangleModel, UnknownXMLDataModel, SceneWriterException,
                   read_scene, write_scene, NS_KARABO)
from ..io_utils import set_numbers
from .utils import temp_file, xml_is_equal

DATA_DIR = op.join(op.abspath(op.dirname(__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')
SCENE_SVG = (
"""<svg xmlns:krb="http://karabo.eu/scene" xmlns:svg="http://www.w3.org/2000/svg" height="768" krb:version="1" width="1024" krb:random="golly" >"""  # noqa
"""<svg:g krb:class="FixedLayout" krb:height="323" krb:width="384" krb:x="106" krb:y="74">"""  # noqa
"""<svg:rect height="60" width="309" x="175" y="125" krb:class="Label" krb:font="Ubuntu,48,-1,5,63,0,0,0,0,0" krb:foreground="#4c4c4c" krb:frameWidth="0" krb:text="Some text" />"""  # noqa
"""<svg:rect fill="none" height="143" stroke="#000000" stroke-dasharray="" stroke-dashoffset="0.0" stroke-linecap="square" stroke-linejoin="bevel" stroke-miterlimit="2.0" stroke-opacity="1.0" stroke-style="1" stroke-width="1.0" width="151" x="106" y="74" />"""  # noqa
"""<svg:line fill="none" stroke="#000000" stroke-dasharray="" stroke-dashoffset="0.0" stroke-linecap="square" stroke-linejoin="bevel" stroke-miterlimit="2.0" stroke-opacity="1.0" stroke-style="1" stroke-width="1.0" x1="397" x2="489" y1="84" y2="396" />"""  # noqa
"""</svg:g>"""
"""<metadata id="some-extra-data"><foo>bar</foo></metadata>"""
"""</svg>"""
)


def _get_file_data(filename):
    with open(filename, 'r') as fp:
        return fp.read()


def _iter_data_files(directory):
    for fn in os.listdir(directory):
        if op.splitext(fn)[-1] == '.svg':
            yield op.join(directory, fn)


def test_set_numbers_name_mapping():
    element = Element('rect')
    model = RectangleModel(x=0.0, y=0, height=10, width=10)
    names = ('x', 'y', 'height', 'width')
    xmlnames = (NS_KARABO + n for n in names[:-1])

    # Normal mapping is tested by the scene writers. Lets make sure the
    # parameter checking works...
    assert_raises(SceneWriterException, set_numbers, names, model, element,
                  xmlnames=xmlnames)


def test_set_numbers_float_conversion():
    element = Element('rect')
    model = RectangleModel(x=0.0, y=0.2, height=10.01, width=10.05)

    # Test rounding of floating point values
    set_numbers(('x', 'y', 'height', 'width'), model, element)
    assert element.get('x') == '0'
    assert element.get('y') == '0.2'
    assert element.get('height') == '10'
    assert element.get('width') == '10.05'


def test_reading():
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)

    assert scene.width == 1024
    assert scene.height == 768
    assert len(scene.children) == 2

    layout = scene.children[0]
    assert len(layout.children) == 3

    child = layout.children[0]
    assert isinstance(child, LabelModel)

    child = layout.children[1]
    assert isinstance(child, RectangleModel)

    child = layout.children[2]
    assert isinstance(child, LineModel)


def test_writing():
    extra_attributes = {NS_KARABO + 'random': 'golly'}
    scene = SceneModel(extra_attributes=extra_attributes)
    layout = FixedLayoutModel(x=106, y=74, height=323, width=384)
    label = LabelModel(
        x=175, y=125, height=60, width=309,
        text='Some text', font='Ubuntu,48,-1,5,63,0,0,0,0,0',
        foreground='#4c4c4c', frame_width=0
    )
    rect = RectangleModel(
        x=106, y=74, height=143, width=151,
        stroke='#000000', stroke_dashoffset=0.0, stroke_linecap='square',
        stroke_linejoin='bevel', stroke_miterlimit=2.0, stroke_opacity=1.0,
        stroke_style=1, stroke_width=1.0
    )
    line = LineModel(
        x1=397, x2=489, y1=84, y2=396,
        stroke="#000000", stroke_dashoffset=0.0, stroke_linecap='square',
        stroke_linejoin='bevel', stroke_miterlimit=2.0, stroke_opacity=1.0,
        stroke_style=1, stroke_width=1.0
    )
    layout.children.extend([label, rect, line])
    scene.children.append(layout)

    unknown_child = UnknownXMLDataModel(tag='foo', data='bar')
    unknown = UnknownXMLDataModel(tag='metadata',
                                  attributes={'id': 'some-extra-data'},
                                  children=[unknown_child])
    scene.children.append(unknown)

    xml = write_scene(scene)
    assert xml_is_equal(SCENE_SVG, xml)


def test_simple_round_trip():
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)

    xml = write_scene(scene)
    assert xml_is_equal(SCENE_SVG, xml)


def test_real_data_reading():
    for fn in _iter_data_files(DATA_DIR):
        read_scene(fn)


def test_real_data_reading_inkscape():
    for fn in _iter_data_files(INKSCAPE_DIR):
        read_scene(fn)


def test_real_data_round_trip():
    for fn in _iter_data_files(DATA_DIR):
        scene = read_scene(fn)
        new_xml = write_scene(scene)
        orig_xml = _get_file_data(fn)

        failmsg = "Scene {} didn't round trip!".format(op.basename(fn))
        assert xml_is_equal(orig_xml, new_xml), failmsg
