# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy import uic
from qtpy.QtWidgets import QDialog
from traits.api import TraitError, Undefined

from karabo.common.api import (
    KARABO_ALARM_HIGH, KARABO_ALARM_LOW, KARABO_WARN_HIGH, KARABO_WARN_LOW)
from karabogui.controllers.validators import SimpleValidator

from .utils import get_dialog_ui

_ALARM_KEYS = [KARABO_ALARM_LOW, KARABO_WARN_LOW,
               KARABO_WARN_HIGH, KARABO_ALARM_HIGH]


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

        self.binding = binding
        self.validator = SimpleValidator(binding)
        self.edit_warnLow.setValidator(self.validator)
        self.edit_warnHigh.setValidator(self.validator)
        self.edit_alarmLow.setValidator(self.validator)
        self.edit_alarmHigh.setValidator(self.validator)

    @property
    def values(self):
        """Get the alarm configuration in order"""
        traits = {}

        active = {}
        for key in _ALARM_KEYS:
            widget = getattr(self, f"edit_{key}")
            value = widget.text()
            value = (Undefined if not value or not widget.hasAcceptableInput()
                     else self.trait_value(value))
            traits[key] = value
            if value is not Undefined:
                active[key] = value

        # Reorder in case of wrong input, but only the active ones
        active = {k: v for k, v in zip(active.keys(), sorted(active.values()))}
        traits.update(active)

        return traits

    def trait_value(self, value):
        try:
            return self.binding.validate_trait("value", value)
        except TraitError:
            return Undefined
