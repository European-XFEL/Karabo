# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..api import StatefulIconWidgetModel
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

STATEFUL_ICON_WIDGETS = ["foo", "bar", "foobar"]


def test_statefulicon_widget():
    model = StatefulIconWidgetModel()
    for name in STATEFUL_ICON_WIDGETS:
        traits = base_widget_traits()
        traits["icon_name"] = name
        model = StatefulIconWidgetModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.icon_name == name
