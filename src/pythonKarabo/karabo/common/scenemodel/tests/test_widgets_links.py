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
from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

FONT_SPEC = "Source Sans Pro,48,-1,5,63,0,0,0,0,0"


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = f"{name} has the wrong value!"
        assert getattr(model, name) == value, msg


def test_device_scene_link_model():
    traits = base_widget_traits()
    traits["target"] = "scene1"
    traits["target_window"] = api.SceneTargetWindow.Dialog
    traits["text"] = "foo"
    traits["font"] = FONT_SPEC
    traits["foreground"] = "#000001"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 2
    model = api.DeviceSceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    # Tests written keys!
    assert_base_traits(read_model)
    assert read_model.text == "foo"
    assert read_model.font == FONT_SPEC
    assert read_model.foreground == "#000001"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 2
    assert read_model.target == "scene1"
    assert read_model.target_window == api.SceneTargetWindow.Dialog
    assert read_model.parent_component == "DisplayComponent"


def test_web_link_model():
    traits = _geometry_traits()
    traits["target"] = "www.xfel.eu"
    traits["text"] = "www.karabo.eu"
    traits["font"] = FONT_SPEC
    traits["foreground"] = "#000001"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 2
    model = api.WebLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == "www.xfel.eu"
    assert read_model.text == "www.karabo.eu"
    assert read_model.font == FONT_SPEC
    assert read_model.foreground == "#000001"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 2
    assert read_model.parent_component == "DisplayComponent"
    assert read_model.keys == []


def test_scene_link_model():
    traits = _geometry_traits()
    traits["target"] = "other.svg"
    traits["text"] = "Some svg"
    traits["font"] = FONT_SPEC
    traits["foreground"] = "#000001"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 2
    traits["target_window"] = api.SceneTargetWindow.Dialog
    model = api.SceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == "other.svg"
    assert read_model.target_window == api.SceneTargetWindow.Dialog
    assert read_model.parent_component == "DisplayComponent"
    assert read_model.text == "Some svg"
    assert read_model.font == FONT_SPEC
    assert read_model.foreground == "#000001"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 2
    assert read_model.keys == []
