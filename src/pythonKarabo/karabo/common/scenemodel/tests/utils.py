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
        rt_scene.assure_svg_data()
    return rt_scene.children[0]
