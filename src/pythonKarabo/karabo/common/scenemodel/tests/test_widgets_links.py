from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

UBUNTU_FONT_SPEC = "Ubuntu,48,-1,5,63,0,0,0,0,0"


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def test_device_scene_link_model():
    traits = base_widget_traits()
    traits["target"] = "scene1"
    traits["target_window"] = api.SceneTargetWindow.Dialog
    traits["text"] = "foo"
    traits["font"] = UBUNTU_FONT_SPEC
    traits["foreground"] = "#000000"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 0
    model = api.DeviceSceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)

    assert read_model.text == "foo"
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == "#000000"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 0
    assert read_model.target == "scene1"
    assert read_model.target_window == api.SceneTargetWindow.Dialog
    assert model.parent_component == "DisplayComponent"


def test_web_link_model():
    traits = _geometry_traits()
    traits["target"] = "www.xfel.eu"
    traits["text"] = "www.karabo.eu"
    traits["font"] = UBUNTU_FONT_SPEC
    traits["foreground"] = "#000000"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 1
    model = api.WebLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == "www.xfel.eu"
    assert read_model.text == "www.karabo.eu"
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == "#000000"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 1


def test_scene_link_model():
    traits = _geometry_traits()
    traits["target"] = "other.svg"
    traits["target_window"] = api.SceneTargetWindow.Dialog
    model = api.SceneLinkModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.target == "other.svg"
    assert read_model.target_window == api.SceneTargetWindow.Dialog