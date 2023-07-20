# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from traits.api import Undefined

from karabogui.binding.api import FloatBinding

from ..alarm_dialog import LABEL_NOTE, AlarmDialog


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


def test_warning_note(gui_app):
    """ Test the warning message when the """
    binding = FloatBinding()
    error_message = "Please make sure the values are in the order:"
    model = {"alarmLow": 1.2, "warnLow": 2.8,
             "warnHigh": 4.2, "alarmHigh": Undefined}
    default_style = "QLabel#label_note {}"
    error_style = "QLabel#label_note {color:red;}"
    dialog = AlarmDialog(binding, model)
    dialog.edit_alarmHigh.setText("0.8")
    order = " < ".join(model.keys())
    text = f"{error_message} {order}"
    assert dialog.label_note.text() == text
    assert dialog.label_note.styleSheet() == error_style

    dialog.edit_alarmHigh.setText("8.2")
    assert dialog.label_note.text() == LABEL_NOTE
    assert dialog.label_note.styleSheet() == default_style

    # Keep a value empty
    dialog.edit_alarmHigh.setText("")
    assert dialog.label_note.text() == LABEL_NOTE
    assert dialog.label_note.styleSheet() == default_style

    # Include only non-empty field names in the warning.
    dialog.edit_warnHigh.setText("0.02")
    expected = f"{error_message} alarmLow < warnLow < warnHigh"
    assert dialog.label_note.text() == expected
    assert dialog.label_note.styleSheet() == error_style
