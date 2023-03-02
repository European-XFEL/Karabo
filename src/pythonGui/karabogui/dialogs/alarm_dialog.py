from qtpy import uic
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QDialog
from traits.api import TraitError, Undefined

from karabo.common.api import (
    KARABO_ALARM_HIGH, KARABO_ALARM_LOW, KARABO_WARN_HIGH, KARABO_WARN_LOW)

from .utils import get_dialog_ui


class AlarmValidator(QValidator):
    """This is a strict binding validator"""

    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self._binding = binding

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if not input:
            return self.Acceptable, input, pos
        try:
            self._binding.validate_trait("value", input)
        except TraitError:
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


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
        self.validator = AlarmValidator(binding)
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
            value = getattr(self, f"edit_{key}").text()
            value = (Undefined if not value
                     else self.binding.validate_trait("value", value))
            traits[key] = value
            if value is not Undefined:
                active[key] = value

        # Reorder in case of wrong input, but only the active ones
        active = {k: v for k, v in zip(active.keys(), sorted(active.values()))}
        traits.update(active)

        return traits
