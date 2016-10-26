from ..api import (DisplayAlignedImageModel, DisplayImageElementModel,
                   DisplayImageModel, ScientificImageModel, WebcamImageModel)
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def _check_simple_image_widget(klass):
    traits = base_widget_traits(parent='DisplayComponent')
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)


def _check_base_image_widget(klass):
    traits = base_widget_traits(parent='DisplayComponent')
    traits.update(_base_image_traits())
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.show_tool_bar is False
    assert read_model.show_color_bar is True
    assert read_model.show_axes is False


def _geometry_traits():
    return {'x': 0, 'y': 0, 'height': 100, 'width': 100}


def _base_image_traits():
    traits = _geometry_traits()
    traits['show_tool_bar'] = False
    traits['show_color_bar'] = True
    traits['show_axes'] = False
    return traits


def test_simple_image_widgets():
    model_classes = (
        DisplayAlignedImageModel, DisplayImageModel, DisplayImageElementModel
        )
    for klass in model_classes:
        yield _check_simple_image_widget, klass


def test_scientific_image_model():
    _check_base_image_widget(ScientificImageModel)


def test_webcam_image_model():
    _check_base_image_widget(WebcamImageModel)
