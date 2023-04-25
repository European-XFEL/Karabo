# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.testing.utils import temp_xml_file

from ..model import SceneModel
from ..modelio import read_scene, write_scene


def assert_base_traits(model):
    assert model.x == 0
    assert model.y == 0
    assert model.width == 100
    assert model.height == 100
    assert model.keys == ["device_id.prop"]


def base_widget_traits():
    traits = {
        "keys": ["device_id.prop"],
        "x": 0,
        "y": 0,
        "height": 100,
        "width": 100,
    }
    return traits


def single_model_from_data(svg_data):
    """Given some SVG data, read a scene model from it an return the first
    child.
    """
    with temp_xml_file(svg_data) as fn:
        return read_scene(fn).children[0]


def single_model_round_trip(model):
    """Given a scene model object, write it to XML and read it back to examine
    the round trip reader/writer behavior.
    """
    scene = SceneModel(children=[model])
    xml = write_scene(scene)
    with temp_xml_file(xml) as fn:
        rt_scene = read_scene(fn)
    return rt_scene.children[0]
