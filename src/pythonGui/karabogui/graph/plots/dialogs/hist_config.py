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
import os

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog


class HistogramDialog(QDialog):
    def __init__(self, config, parent=None):
        super().__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'hist_config.ui')
        uic.loadUi(ui_path, self)

        self.ui_start.setValue(config['start'])
        self.ui_stop.setValue(config['stop'])
        state = config['auto']
        self.ui_auto.setChecked(state)
        self.ui_start.setEnabled(not state)
        self.ui_stop.setEnabled(not state)
        self.ui_bins.setValue(config['bins'])

        self.ui_auto.stateChanged.connect(self.check_scales)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @Slot()
    def check_scales(self):
        state = self.ui_auto.isChecked()
        self.ui_start.setEnabled(not state)
        self.ui_stop.setEnabled(not state)

    @property
    def settings(self):
        config = {
            "start": self.ui_start.value(),
            "stop": self.ui_stop.value(),
            "auto": self.ui_auto.isChecked(),
            "bins": self.ui_bins.value()}

        return config

    @staticmethod
    def get(configuration, parent=None):
        dialog = HistogramDialog(configuration, parent)
        result = dialog.exec() == QDialog.Accepted
        content = {}
        content.update(dialog.settings)

        return content, result
