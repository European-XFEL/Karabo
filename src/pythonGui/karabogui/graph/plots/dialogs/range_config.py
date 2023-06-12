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
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog

from karabogui.validators import NumberValidator

DECIMALS = 6


class RangeDialog(QDialog):
    def __init__(self, config=None, actual=None, axis=0, parent=None):
        super().__init__(parent)
        self.setModal(False)
        ui_path = Path(__file__).parent.joinpath("range_config.ui")
        uic.loadUi(ui_path, self)
        self.get_axis_notation(axis=axis)
        self.ui_min.setValidator(NumberValidator(decimals=DECIMALS))
        self.ui_max.setValidator(NumberValidator(decimals=DECIMALS))

        # Show the actual view!
        self.ui_view_min.setText(f"{actual[self.axis_min]:.2e}")
        self.ui_view_max.setText(f"{actual[self.axis_max]:.2e}")

        # Load the model settings!
        self.ui_min.setText(f"{config[self.axis_min]}")
        self.ui_max.setText(f"{config[self.axis_max]}")

        state = config[self.axis_auto]
        self.ui_autorange.setChecked(state)
        self.ui_autorange.toggled.connect(self._check_box_triggered)
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)
        if not state:
            self.ui_min.setFocus()
            self.ui_min.selectAll()

    def get_axis_notation(self, axis):
        axis = "x" if axis == 0 else "y"
        self.axis_min = f"{axis}_min"
        self.axis_max = f"{axis}_max"
        self.axis_auto = f"{axis}_autorange"
        self.ui_axis.setText(f"Axis View: {axis}-axis")

    @Slot(bool)
    def _check_box_triggered(self, state):
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)
        if not state:
            self.ui_min.setFocus()
            self.ui_min.selectAll()

    @property
    def limits(self):
        limits = [float(self.ui_min.text()), float(self.ui_max.text())]
        axis_min = min(limits)
        axis_max = max(limits)
        config = {
            self.axis_min: axis_min,
            self.axis_max: axis_max,
            self.axis_auto: self.ui_autorange.isChecked()}

        return config

    @staticmethod
    def get(model, actual, axis=0, parent=None):
        dialog = RangeDialog(model, actual, axis, parent)
        result = dialog.exec() == QDialog.Accepted
        content = {}
        content.update(dialog.limits)

        return content, result
