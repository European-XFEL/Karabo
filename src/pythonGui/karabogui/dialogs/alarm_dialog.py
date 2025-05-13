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
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog
from traits.api import TraitError, Undefined

from karabogui.controllers.validators import SimpleValidator

from .utils import get_dialog_ui

_ALARM_KEYS = ["alarmLow", "warnLow", "warnHigh", "alarmHigh"]

LABEL_NOTE = ("Note: In order to deactivate an alarm setting, please leave "
              "the edit field empty.")


class AlarmDialog(QDialog):

    def __init__(self, binding, config, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("alarm_dialog.ui"), self)
        for key, value in config.items():
            if key not in _ALARM_KEYS:
                continue
            value = "" if value is Undefined else str(value)
            widget = getattr(self, f"edit_{key}")
            widget.setText(value)
            widget.textChanged.connect(self.validate_the_order)

        self.binding = binding
        self.validator = SimpleValidator(binding)
        self.edit_warnLow.setValidator(self.validator)
        self.edit_warnHigh.setValidator(self.validator)
        self.edit_alarmLow.setValidator(self.validator)
        self.edit_alarmHigh.setValidator(self.validator)

    @property
    def values(self):
        """Get the alarm configuration in order"""
        traits = self._get_traits()

        active = {}
        for key, value in traits.items():
            if value is not Undefined:
                active[key] = value

        # Reorder in case of wrong input, but only the active ones
        active = {k: v for k, v in zip(active.keys(), sorted(active.values()))}
        traits.update(active)

        return traits

    def _get_traits(self):
        traits = {}
        for key in _ALARM_KEYS:
            widget = getattr(self, f"edit_{key}")
            value = widget.text()
            value = (Undefined if not value or not widget.hasAcceptableInput()
                     else self.trait_value(value))
            traits[key] = value
        return traits

    def trait_value(self, value):
        try:
            return self.binding.validate_trait("value", value)
        except TraitError:
            return Undefined

    @Slot()
    def validate_the_order(self):
        """
        Validate the order of all the limit values. It should always be
           alarmLow < warnLow < warnHigh <  alarmHigh.

        Empty values are ignored. For example if "warnLow" is not defined
        then, the condition is
           alarmLow < warnHigh <  alarmHigh.

        A message is displayed in red color when the values do not follow
        the order.
        """
        traits = self._get_traits()
        actives = {k: v for k, v in traits.items() if v is not Undefined}
        values = list(actives.values())
        valid_order = values == sorted(values, key=lambda x: float(x))
        if not valid_order:
            order = " < ".join(actives.keys())
            message = f"Please make sure the values are in the order: {order}"
            style = "QLabel#label_note {color:red;}"
        else:
            message = LABEL_NOTE
            style = "QLabel#label_note {}"
        self.label_note.setText(message)
        self.label_note.setStyleSheet(style)
