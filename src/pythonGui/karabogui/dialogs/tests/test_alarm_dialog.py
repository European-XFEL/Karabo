# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from traits.api import Undefined

from karabogui.binding.api import FloatBinding

from ..alarm_dialog import AlarmDialog


def test_alarm_configuration_dialog(gui_app):
    binding = FloatBinding()
    config = {}
    dialog = AlarmDialog(binding, config)

    assert dialog.edit_alarmLow.text() == ""
    assert dialog.edit_warnLow.text() == ""
    assert dialog.edit_warnHigh.text() == ""
    assert dialog.edit_alarmHigh.text() == ""

    # Check the return values
    for v in dialog.values.values():
        assert v is Undefined

    model = {"alarmLow": 1.2}
    dialog = AlarmDialog(binding, model)
    assert dialog.edit_alarmLow.text() == "1.2"
    assert dialog.edit_warnLow.text() == ""
    assert dialog.edit_warnHigh.text() == ""
    assert dialog.edit_alarmHigh.text() == ""
    assert dialog.values["alarmLow"] == 1.2

    # Test a full model
    model = {"alarmLow": 1.0, "warnLow": 1.2,
             "warnHigh": 2.1, "alarmHigh": 5.7}
    dialog = AlarmDialog(binding, model)
    assert dialog.edit_alarmLow.text() == "1.0"
    assert dialog.edit_warnLow.text() == "1.2"
    assert dialog.edit_warnHigh.text() == "2.1"
    assert dialog.edit_alarmHigh.text() == "5.7"
    assert dialog.values["alarmLow"] == 1.0
    assert dialog.values["warnLow"] == 1.2
    assert dialog.values["warnHigh"] == 2.1
    assert dialog.values["alarmHigh"] == 5.7

    # Test an unordered model, this cannot happen on the field, because we
    # order!
    model = {"alarmLow": 1.2, "warnLow": 2.8,
             "warnHigh": 4.2, "alarmHigh": 0.8}
    dialog = AlarmDialog(binding, model)
    assert dialog.edit_alarmLow.text() == "1.2"
    assert dialog.edit_warnLow.text() == "2.8"
    assert dialog.edit_warnHigh.text() == "4.2"
    assert dialog.edit_alarmHigh.text() == "0.8"
    assert dialog.values["alarmLow"] == 0.8
    assert dialog.values["warnLow"] == 1.2
    assert dialog.values["warnHigh"] == 2.8
    assert dialog.values["alarmHigh"] == 4.2

    # Test unordered but with empty setting
    model = {"alarmLow": 1.2, "warnHigh": 4.2, "alarmHigh": 0.8}
    dialog = AlarmDialog(binding, model)
    assert dialog.edit_alarmLow.text() == "1.2"
    assert dialog.edit_warnLow.text() == ""
    assert dialog.edit_warnHigh.text() == "4.2"
    assert dialog.edit_alarmHigh.text() == "0.8"
    assert dialog.values["alarmLow"] == 0.8
    assert dialog.values["warnLow"] is Undefined
    assert dialog.values["warnHigh"] == 1.2
    assert dialog.values["alarmHigh"] == 4.2

    dialog.destroy()
