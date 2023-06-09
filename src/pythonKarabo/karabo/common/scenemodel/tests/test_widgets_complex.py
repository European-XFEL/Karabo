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
from pytest import raises as assert_raises
from traits.api import TraitError

from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = f"{name} has the wrong value!"
        assert getattr(model, name) == value, msg


def test_doubleline_edit():
    traits = base_widget_traits()
    traits["decimals"] = 5
    model = api.DoubleLineEditModel(**traits)
    assert model.parent_component == "EditableApplyLaterComponent"
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.decimals == 5
    assert read_model.parent_component == model.parent_component


def test_color_bool_widget():
    traits = base_widget_traits()
    traits["invert"] = True
    model = api.ColorBoolModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert


def test_display_command():
    traits = _geometry_traits()
    traits["requires_confirmation"] = True
    traits["font_size"] = 12
    traits["font_weight"] = "bold"
    model = api.DisplayCommandModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.requires_confirmation
    assert read_model.font_size == 12
    assert read_model.font_weight == "bold"


def test_display_icon_command():
    traits = _geometry_traits()
    traits["icon_name"] = "stop"
    model = api.DisplayIconCommandModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.icon_name == "stop"


def test_error_bool_widget():
    traits = base_widget_traits()
    traits["invert"] = True
    model = api.ErrorBoolModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert


def test_display_progress_bar_widget():
    traits = base_widget_traits()
    traits["is_vertical"] = True
    model = api.DisplayProgressBarModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.is_vertical


def test_display_state_color_widget():
    traits = base_widget_traits()
    traits["show_string"] = True
    model = api.DisplayStateColorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.show_string

    assert model.font_size == api.SCENE_FONT_SIZE
    assert model.font_weight == api.SCENE_FONT_WEIGHT

    # Check valid input
    input_size = 7
    input_weight = "bold"
    model = api.DisplayLabelModel(
        font_size=input_size, font_weight=input_weight
    )
    assert model.font_size == input_size
    assert model.font_weight == input_weight

    # Check invalid input
    assert_raises(TraitError, api.DisplayLabelModel, font_size=1)
    assert_raises(TraitError, api.DisplayLabelModel, font_weight="foo")


def test_evaluator_widget():
    traits = base_widget_traits()
    traits["expression"] = "x"
    model = api.EvaluatorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.expression == "x"


def test_float_spinbox_widget():
    traits = base_widget_traits()
    traits["step"] = 1.5
    traits["font_size"] = 12
    traits["font_weight"] = "bold"
    model = api.FloatSpinBoxModel(**traits)
    assert model.parent_component == "EditableApplyLaterComponent"
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.step == 1.5
    assert read_model.parent_component == model.parent_component
    assert read_model.font_size == 12
    assert read_model.font_weight == "bold"


def test_monitor_widget():
    traits = base_widget_traits()
    traits["filename"] = "foo.log"
    traits["interval"] = 1.5
    model = api.MonitorModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.filename == "foo.log"
    assert read_model.interval == 1.5


def test_single_bit_widget():
    traits = base_widget_traits()
    traits["invert"] = True
    traits["bit"] = 42
    model = api.SingleBitModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.invert
    assert read_model.bit == 42


def test_table_element_widget():
    for klass_name in ("DisplayTableElement", "EditableTableElement"):
        traits = base_widget_traits()
        traits["klass"] = klass_name
        traits["resizeToContents"] = True
        model = api.TableElementModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.resizeToContents


def test_filter_table_element_widget():
    for klass_name in (
            "DisplayFilterTableElement",
            "EditableFilterTableElement",
    ):
        traits = base_widget_traits()
        traits["klass"] = klass_name
        traits["resizeToContents"] = True
        traits["sortingEnabled"] = True
        traits["showFilterKeyColumn"] = True
        model = api.FilterTableElementModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.resizeToContents
        assert read_model.sortingEnabled
        assert read_model.showFilterKeyColumn
