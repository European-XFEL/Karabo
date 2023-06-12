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
from qtpy.QtWidgets import QDialog


class TransformDialog(QDialog):
    def __init__(self, config, parent=None):
        super().__init__(parent)
        self.setModal(False)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'transform_config.ui')
        uic.loadUi(ui_path, self)
        self.ui_offset.setValue(config['offset'])
        self.ui_step.setValue(config['step'])
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def transformations(self):
        config = {
            "offset": self.ui_offset.value(),
            "step": self.ui_step.value()}

        return config

    @staticmethod
    def get(configuration, parent=None):
        dialog = TransformDialog(configuration, parent)
        result = dialog.exec() == QDialog.Accepted
        content = {}
        content.update(dialog.transformations)

        return content, result
