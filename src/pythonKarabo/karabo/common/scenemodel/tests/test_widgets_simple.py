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
from traits.api import TraitError, Undefined

from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

UBUNTU_FONT_SPEC = "Ubuntu,48,-1,5,63,0,0,0,0,0"


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = f"{name} has the wrong value!"
        assert getattr(model, name) == value, msg


def _check_display_editable_widget(klass):
    suffix = klass.__name__[: -len("Model")]
    for prefix in ("Display", "Editable"):
        traits = base_widget_traits()
        klass_name = prefix + suffix
        traits["klass"] = klass_name
        model = klass(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == klass_name
        assert read_model.parent_component.startswith(prefix)


def _check_empty_widget(klass):
    traits = base_widget_traits()
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)


def _check_editable_empty_widget(klass):
    traits = base_widget_traits()
    model = klass(**traits)
    assert model.parent_component == "EditableApplyLaterComponent"
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.parent_component == model.parent_component


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def test_all_empty_widgets():
    model_classes = (
        api.DisplayLabelModel,
        api.DisplayListModel,
        api.WidgetNodeModel,
        api.InstanceStatusModel,
    )
    for klass in model_classes:
        _check_empty_widget(klass)


def test_display_editable_widgets():
    model_classes = (
        api.CheckBoxModel,
        api.LineEditModel,
    )
    for klass in model_classes:
        _check_display_editable_widget(klass)


def test_editable_simple_model():
    model_classes = (
        api.EditableComboBoxModel,
        api.EditableChoiceElementModel,
        api.EditableListModel,
        api.EditableListElementModel,
        api.EditableSpinBoxModel,
        api.HexadecimalModel,
        api.IntLineEditModel,
        api.EditableRegexListModel,
        api.EditableRegexModel,
    )
    for klass in model_classes:
        _check_editable_empty_widget(klass)


def test_missing_parent_component():
    traits = base_widget_traits()
    traits["parent_component"] = ""  # explicitly empty!
    model = api.DisplayLabelModel(**traits)
    assert_raises(api.SceneWriterException, single_model_round_trip, model)


def test_displaylabel_model():
    # Check default model
    default_model = api.DisplayLabelModel()
    assert default_model.font_size == api.SCENE_FONT_SIZE
    assert default_model.font_weight == api.SCENE_FONT_WEIGHT

    # Check valid input
    input_size = 7
    input_weight = "bold"
    valid_model = api.DisplayLabelModel(
        font_size=input_size, font_weight=input_weight
    )
    assert valid_model.font_size == input_size
    assert valid_model.font_weight == input_weight

    # Check invalid input
    assert_raises(TraitError, api.DisplayLabelModel, font_size=1)
    assert_raises(TraitError, api.DisplayLabelModel, font_weight="foo")


def test_label_model():
    traits = _geometry_traits()
    traits["text"] = "foo"
    traits["font"] = UBUNTU_FONT_SPEC
    traits["foreground"] = "#000000"
    traits["background"] = "#ffffff"
    traits["frame_width"] = 0
    traits["alignh"] = 2

    model = api.LabelModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.text == "foo"
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == "#000000"
    assert read_model.background == "#ffffff"
    assert read_model.frame_width == 0
    assert read_model.alignh == 2

    assert_raises(TraitError, api.LabelModel, alignh=3)
    assert_raises(TraitError, api.LabelModel, alignh=5)


def test_spinbox_model():
    traits = _geometry_traits()
    traits["font_size"] = 12
    traits["font_weight"] = "bold"

    model = api.EditableSpinBoxModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.font_size == 12
    assert read_model.font_weight == "bold"


def test_sticker_model():
    traits = _geometry_traits()
    traits["text"] = "foo"
    traits["font"] = UBUNTU_FONT_SPEC
    traits["foreground"] = "#000000"
    traits["background"] = "#ffffff"
    model = api.StickerModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.text == "foo"
    assert read_model.font == UBUNTU_FONT_SPEC
    assert read_model.foreground == "#000000"
    assert read_model.background == "#ffffff"


def test_tickslider():
    traits = _geometry_traits()
    traits["ticks"] = 500
    traits["show_value"] = False
    model = api.TickSliderModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.ticks == 500
    assert read_model.show_value is False


def test_timelabel():
    traits = _geometry_traits()
    traits["time_format"] = "%H:%M:%S"
    traits["font_size"] = 18
    traits["font_weight"] = "bold"
    model = api.DisplayTimeModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.time_format == "%H:%M:%S"
    assert read_model.font_size == 18
    assert read_model.font_weight == "bold"


def test_list():
    traits = _geometry_traits()
    traits["font_size"] = 18
    traits["font_weight"] = "bold"

    model = api.DisplayListModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)

    assert read_model.font_size == 18
    assert read_model.font_weight == "bold"


def test_display_float_alarm_models():
    # Check default model
    for model_klass in [api.DisplayFloatModel, api.DisplayAlarmFloatModel]:
        default_model = model_klass()
        assert default_model.font_size == api.SCENE_FONT_SIZE
        assert default_model.font_weight == api.SCENE_FONT_WEIGHT
        assert default_model.fmt == "g"
        assert default_model.decimals == "8"

        # Check valid input, labels
        input_size = 7
        input_weight = "bold"
        input_fmt = "f"
        input_decimals = "4"
        valid_model = model_klass(
            font_size=input_size, font_weight=input_weight,
            decimals=input_decimals,
            fmt=input_fmt)
        assert valid_model.font_size == input_size
        assert valid_model.font_weight == input_weight
        assert valid_model.fmt == input_fmt
        assert valid_model.decimals == input_decimals

        # Check invalid input
        assert_raises(TraitError, api.DisplayFloatModel, font_size=1)
        assert_raises(TraitError, api.DisplayFloatModel, font_weight="foo")

        # Check round trip
        read_model = single_model_round_trip(valid_model)
        assert read_model.font_size == input_size
        assert read_model.font_weight == input_weight
        assert read_model.fmt == input_fmt
        assert read_model.decimals == input_decimals

    model = api.DisplayAlarmFloatModel(alarmLow=1.2, warnHigh=6.5,
                                       warnLow=5.2, alarmHigh=7.9)
    read_model = single_model_round_trip(model)
    assert read_model.alarmLow == 1.2
    assert read_model.warnLow == 5.2
    assert read_model.warnHigh == 6.5
    assert read_model.alarmHigh == 7.9

    model = api.DisplayAlarmFloatModel(alarmLow=1.2, alarmHigh=7.9)
    read_model = single_model_round_trip(model)
    assert read_model.alarmLow == 1.2
    assert read_model.warnLow is Undefined
    assert read_model.warnHigh is Undefined
    assert read_model.alarmHigh == 7.9

    # And the integers ...

    model = api.DisplayAlarmIntegerModel(alarmLow=1, warnHigh=6,
                                         warnLow=5, alarmHigh=7)
    read_model = single_model_round_trip(model)
    assert read_model.alarmLow == 1
    assert read_model.warnLow == 5
    assert read_model.warnHigh == 6
    assert read_model.alarmHigh == 7

    model = api.DisplayAlarmIntegerModel(alarmLow=1, alarmHigh=8)
    read_model = single_model_round_trip(model)
    assert read_model.alarmLow == 1
    assert read_model.warnLow is Undefined
    assert read_model.warnHigh is Undefined
    assert read_model.alarmHigh == 8


def test_popup_button_model():
    traits = _geometry_traits()
    traits["text"] = "Karabo Popup Sticker"
    traits["info_type"] = "Critical Info"
    model = api.PopupButtonModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.text == "Karabo Popup Sticker"
    assert read_model.info_type == "Critical Info"
