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
from karabo.common.utils import get_arrowhead_points
from karabo.testing.utils import temp_xml_file

from ..model import UnknownXMLDataModel
from ..modelio import read_scene
from ..shapes import ArrowPolygonModel, LineModel, PathModel, RectangleModel
from .utils import single_model_round_trip


def _assert_base_traits(model):
    traits = _base_shape_traits()
    for name, value in traits.items():
        msg = f"{name} has the wrong value!"
        assert getattr(model, name) == value, msg


def _base_shape_traits():
    return {
        "stroke": "#ff00ff",
        "stroke_opacity": 0.5,
        "stroke_linecap": "square",
        "stroke_dashoffset": 0.5,
        "stroke_width": 2.0,
        "stroke_dasharray": [1.0, 2.0, 3.0],
        "stroke_style": 1,
        "stroke_linejoin": "round",
        "stroke_miterlimit": 5.0,
        "fill": "#424242",
        "fill_opacity": 1.0,
    }


def test_line_model():
    traits = _base_shape_traits()
    traits.update({"x1": 0, "y1": 0, "x2": 2, "y2": 2})
    model = LineModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.x1 == 0
    assert read_model.y1 == 0
    assert read_model.x2 == 2
    assert read_model.y2 == 2


def test_path_model():
    traits = _base_shape_traits()
    traits["svg_data"] = "<svg:line/>"
    model = PathModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.svg_data == "<svg:line/>"


def test_rectangle_model():
    traits = _base_shape_traits()
    traits.update({"x": 0, "y": 0, "width": 10, "height": 10})
    model = RectangleModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_base_traits(read_model)
    assert read_model.x == 0
    assert read_model.y == 0
    assert read_model.width == 10
    assert read_model.height == 10


def test_default_fill():
    SCENE_SVG = (
        """<svg xmlns:svg="http://www.w3.org/2000/svg">"""
        """<svg:rect style="stroke:#000000" x="397" y="84" height="10" width="40" />"""  # noqa
        """</svg>"""
    )
    with temp_xml_file(SCENE_SVG) as fn:
        scene = read_scene(fn)
    rect_model = scene.children[0]

    assert rect_model.fill == "black"


def test_style_attributes_with_units():
    SCENE_SVG = (
        """<svg xmlns:krb="http://karabo.eu/scene" xmlns:svg="http://www.w3.org/2000/svg" height="768" krb:version="1" width="1024">"""  # noqa
        """<svg:line stroke="#000000" stroke-dashoffset="0.1in" stroke-width="1.0cm" x1="397" x2="489" y1="84" y2="396" />"""  # noqa
        """</svg>"""
    )
    with temp_xml_file(SCENE_SVG) as fn:
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
    with temp_xml_file(SCENE_SVG) as fn:
        scene = read_scene(fn)
    line_model = scene.children[0]
    assert line_model.stroke == "#000000"
    assert line_model.stroke_width == 1.0
    assert line_model.stroke_dashoffset == 0.1
    assert line_model.stroke_miterlimit == 2.0


def test_svg_arrow():
    SCENE_SVG = (
        """<svg xmlns:svg="http://www.w3.org/2000/svg" width="600" height="100">"""  # noqa
        """<svg:line x1="295" y1="50" x2="95" y2="75" stroke="#000" stroke-width="5" marker-end="url(#arrow)"/>"""  # noqa
        """<svg:defs>"""
        """<svg:marker id="arrow" markerWidth="10" markerHeight="10" refX="0" refY="3" orient="auto" markerUnits="strokeWidth">"""  # noqa
        """<svg:path d="M0,0 L0,6 L9,3 z" fill="#f00"/>"""  # noqa
        """</svg:marker>"""
        """</svg:defs>"""
        """</svg>"""
    )
    with temp_xml_file(SCENE_SVG) as fn:
        scene_model = read_scene(fn)

        # Check if the scene model contains a defs model and an arrow model
        assert len(scene_model.children) == 2
        for child in scene_model.children:
            assert isinstance(child, (ArrowPolygonModel, UnknownXMLDataModel))

        # Check arrow model
        arrow_model = _get_child_model(scene_model, ArrowPolygonModel)
        assert arrow_model.x1 == 295
        assert arrow_model.y1 == 50
        assert arrow_model.x2 == 95
        assert arrow_model.y2 == 75
        hx1, hy1, hx2, hy2 = get_arrowhead_points(
            arrow_model.x1, arrow_model.y1, arrow_model.x2, arrow_model.y2)

        assert hx1 == arrow_model.hx1
        assert hy1 == arrow_model.hy1
        assert hx2 == arrow_model.hx2
        assert hy2 == arrow_model.hy2

        assert arrow_model.stroke == "#000"
        assert arrow_model.stroke_width == 5

        read_model = single_model_round_trip(arrow_model)
        assert read_model.x1 == arrow_model.x1
        assert read_model.x2 == arrow_model.x2
        assert read_model.y1 == arrow_model.y1
        assert read_model.y2 == arrow_model.y2


def test_arrow_with_polygon():
    # A round trip for the model
    traits = _base_shape_traits()
    traits.update({"x1": 0, "y1": 1, "x2": 2, "y2": 3})
    arrow_model = ArrowPolygonModel(**traits)
    read_model = single_model_round_trip(arrow_model)

    assert read_model.x1 == 0
    assert read_model.y1 == 1
    assert read_model.x2 == 2
    assert read_model.y2 == 3

    assert arrow_model.hx1 == read_model.hx1
    assert arrow_model.hy1 == read_model.hy1
    assert arrow_model.hx2 == read_model.hx2
    assert arrow_model.hy2 == read_model.hy2


def test_polygon_arrow_from_xml_def():

    # SVG from UnknownWidget in Karabo2.20 for the polygon arrow created in
    # Karabo2.21
    SCENE_SVG = (
        """<svg:svg xmlns:krb="http://karabo.eu/scene" xmlns:svg='http://www.w3.org/2000/svg'
         krb:version="2" krb:uuid="7bf41ba7-e675-46d4-82d3-a56b5662e965" height="768" width="1024">
    <svg:rect krb:class="ArrowPolygonModel" krb:widget="ArrowPolygonModel" krb:keys=""
              x="50" y="130" width="150" height="10" stroke="#000000" stroke-opacity="1.0"
              stroke-linecap="butt" stroke-dashoffset="0.0" stroke-width="1.0"
              stroke-dasharray="" stroke-style="1" stroke-linejoin="miter"
              stroke-miterlimit="4.0" fill="#000000" fill-opacity="1.0" x1="50" y1="130"
              x2="200" y2="130" hx1="190" hy1="133" hx2="190" hy2="127">
    </svg:rect>
    </svg:svg>""" # noqa
    )
    with temp_xml_file(SCENE_SVG) as fn:
        scene_model = read_scene(fn)
        assert len(scene_model.children) == 1
        arrow = scene_model.children[0]
        assert isinstance(arrow, ArrowPolygonModel)
        assert arrow.x == 50
        assert arrow.y == 130
        assert arrow.x1 == 50
        assert arrow.y1 == 130
        assert arrow.x2 == 200
        assert arrow.y2 == 130

        assert arrow.hx1 == 190
        assert arrow.hy1 == 133
        assert arrow.hx2 == 190
        assert arrow.hy2 == 127


def test_svg_defs():
    SCENE_SVG = (
        """<svg xmlns:svg="http://www.w3.org/2000/svg" width="600" height="100">"""  # noqa
        """<svg:defs id="arrow" style="foo">"""
        """<svg:marker markerWidth="10" markerHeight="10" refX="0" refY="3" orient="auto" markerUnits="strokeWidth">"""  # noqa
        """<svg:path d="M0,0 L0,6 L9,3 z" fill="#f00"/>"""  # noqa
        """</svg:marker>"""
        """</svg:defs>"""
        """</svg>"""
    )

    with temp_xml_file(SCENE_SVG) as fn:
        scene_model = read_scene(fn)
    assert len(scene_model.children) == 1
    assert isinstance(scene_model.children[0], UnknownXMLDataModel)


def test_svg_unknowns():
    SCENE_SVG = (
        """<svg xmlns:svg="http://www.w3.org/2000/svg" width="600" height="100">"""  # noqa
        """<svg:linearGradient gradientTransform="translate(88.4376,-184.98619)" id="linearGradient"/>"""  # noqa
        """</svg>"""
    )

    with temp_xml_file(SCENE_SVG) as fn:
        scene_model = read_scene(fn)

    assert len(scene_model.children) == 1
    unknown_model = scene_model.children[0]
    assert isinstance(unknown_model, UnknownXMLDataModel)
    assert unknown_model.id == "linearGradient"
    attrib = {"gradientTransform": "translate(88.4376,-184.98619)"}
    assert unknown_model.attributes == attrib

    read_model = single_model_round_trip(unknown_model)
    assert read_model.id == unknown_model.id
    assert read_model.attributes == unknown_model.attributes


def _get_child_model(model, klass):
    for child in model.children:
        if isinstance(child, klass):
            return child
