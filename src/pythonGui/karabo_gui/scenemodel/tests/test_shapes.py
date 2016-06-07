from ..io import read_scene
from ..shapes import LineModel, PathModel, RectangleModel
from .utils import single_model_round_trip, temp_file


def _assert_base_traits(model):
    traits = _base_shape_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _base_shape_traits():
    return {
        'stroke': '#ff00ff',
        'stroke_opacity': 0.5,
        'stroke_linecap': 'square',
        'stroke_dashoffset': 0.5,
        'stroke_width': 2.0,
        'stroke_dasharray': [1.0, 2.0, 3.0],
        'stroke_style': 1,
        'stroke_linejoin': 'round',
        'stroke_miterlimit': 5.0,
        'fill': '#424242',
        'fill_opacity': 1.0,
    }


def test_line_model():
    traits = _base_shape_traits()
    traits.update({'x1': 0, 'y1': 0, 'x2': 2, 'y2': 2})
    model = LineModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.x1 == 0
    assert read_model.y1 == 0
    assert read_model.x2 == 2
    assert read_model.y2 == 2


def test_path_model():
    traits = _base_shape_traits()
    traits['svg_data'] = '<svg:line/>'
    model = PathModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.svg_data == '<svg:line/>'


def test_rectangle_model():
    traits = _base_shape_traits()
    traits.update({'x': 0, 'y': 0, 'width': 10, 'height': 10})
    model = RectangleModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.x == 0
    assert read_model.y == 0
    assert read_model.width == 10
    assert read_model.height == 10


def test_style_attributes_with_units():
    SCENE_SVG = (
        """<svg xmlns:krb="http://karabo.eu/scene" xmlns:svg="http://www.w3.org/2000/svg" height="768" krb:version="1" width="1024">"""  # noqa
        """<svg:line stroke="#000000" stroke-dashoffset="0.1in" stroke-width="1.0cm" x1="397" x2="489" y1="84" y2="396" />"""  # noqa
        """</svg>"""
    )
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)
    line_model = scene.children[0]
    assert line_model.stroke_width == 35.43307
    assert line_model.stroke_dashoffset == 0.1 * 90


def test_alternate_style_def():
    SCENE_SVG = (
        """<svg xmlns:krb="http://karabo.eu/scene" xmlns:svg="http://www.w3.org/2000/svg" height="768" krb:version="1" width="1024">"""  # noqa
        """<svg:line style="stroke:#000000;stroke-dashoffset:0.1;stroke-miterlimit:2.0;stroke-width:1.0" x1="397" x2="489" y1="84" y2="396" />"""  # noqa
        """</svg>"""
    )
    with temp_file(SCENE_SVG) as fn:
        scene = read_scene(fn)
    line_model = scene.children[0]
    assert line_model.stroke == '#000000'
    assert line_model.stroke_width == 1.0
    assert line_model.stroke_dashoffset == 0.1
    assert line_model.stroke_miterlimit == 2.0
