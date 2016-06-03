import os
import os.path as op

from ..api import (SceneModel, FixedLayoutModel, LabelModel, LineModel,
                   RectangleModel, read_scene, write_scene)
from .utils import temp_file, xml_is_equal

DATA_DIR = op.join(op.abspath(op.dirname(__file__)), 'data')
SCENE_SVG = (
"""<svg xmlns:krb="http://karabo.eu/scene" xmlns:svg="http://www.w3.org/2000/svg" height="768" version="1" width="1024">"""  # noqa
"""<svg:g krb:class="FixedLayout" krb:height="323" krb:width="384" krb:x="106" krb:y="74">"""  # noqa
"""<svg:rect height="60" width="309" x="175" y="125" krb:class="Label" krb:font="Ubuntu,48,-1,5,63,0,0,0,0,0" krb:foreground="#4c4c4c" krb:frameWidth="0" krb:text="Some text" />"""  # noqa
"""<svg:rect fill="none" height="143" stroke="#000000" stroke-dasharray="" stroke-dashoffset="0.0" stroke-linecap="square" stroke-linejoin="bevel" stroke-miterlimit="2.0" stroke-opacity="1.0" stroke-style="1" stroke-width="1.0" width="151" x="106" y="74" />"""  # noqa
"""<svg:line fill="none" stroke="#000000" stroke-dasharray="" stroke-dashoffset="0.0" stroke-linecap="square" stroke-linejoin="bevel" stroke-miterlimit="2.0" stroke-opacity="1.0" stroke-style="1" stroke-width="1.0" x1="397" x2="489" y1="84" y2="396" />"""  # noqa
"""</svg:g>"""
"""</svg>"""
)


def test_reading():
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)

    assert scene.width == 1024
    assert scene.height == 768
    assert len(scene.children) == 1

    layout = scene.children[0]
    assert len(layout.children) == 3

    child = layout.children[0]
    assert isinstance(child, LabelModel)

    child = layout.children[1]
    assert isinstance(child, RectangleModel)

    child = layout.children[2]
    assert isinstance(child, LineModel)


def test_writing():
    scene = SceneModel()
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

    xml = write_scene(scene)
    assert xml_is_equal(SCENE_SVG, xml)


def test_round_trip():
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)

    xml = write_scene(scene)
    assert xml == SCENE_SVG.encode('utf-8')


def test_real_data_reading():
    data_file_paths = [op.join(DATA_DIR, fn) for fn in os.listdir(DATA_DIR)
                       if op.splitext(fn)[-1] == '.svg']

    for fn in data_file_paths:
        read_scene(fn)
