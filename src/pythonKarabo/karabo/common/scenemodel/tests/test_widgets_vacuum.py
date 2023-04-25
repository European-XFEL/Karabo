# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..api import VacuumWidgetModel
from ..widgets.vacuum import VACUUM_WIDGETS
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)


def test_vacuum_widget():
    model = VacuumWidgetModel()
    for name in VACUUM_WIDGETS:
        traits = base_widget_traits()
        traits["klass"] = name
        model = VacuumWidgetModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == name
