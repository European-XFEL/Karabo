from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _check_simple_image_widget(old_klass, new_klass):
    traits = base_widget_traits()
    model = old_klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert isinstance(read_model, new_klass)


def _check_base_webcam_widget(klass):
    traits = base_widget_traits()
    traits.update(_base_image_traits())
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert isinstance(read_model, api.WebCamGraphModel)


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def _base_image_traits():
    traits = _geometry_traits()
    traits["show_tool_bar"] = False
    traits["show_color_bar"] = True
    traits["show_axes"] = False
    return traits


def test_deprecated_image_widgets():
    _check_simple_image_widget(
        old_klass=api.DisplayAlignedImageModel,
        new_klass=api.DetectorGraphModel,
    )
    _check_simple_image_widget(
        old_klass=api.DisplayImageModel, new_klass=api.ImageGraphModel
    )
    _check_simple_image_widget(
        old_klass=api.DisplayImageElementModel, new_klass=api.WebCamGraphModel
    )


def test_deprecated_webcam_model():
    _check_base_webcam_widget(api.ScientificImageModel)
    _check_base_webcam_widget(api.WebcamImageModel)
