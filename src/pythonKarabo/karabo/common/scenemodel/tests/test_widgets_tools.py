# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)


def test_svg_renderer():
    traits = base_widget_traits()
    traits["image"] = api.convert_to_svg_image("svg", b"asd")
    model = api.ImageRendererModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.image == "data:image/svg;base64,YXNk"
