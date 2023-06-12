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

from karabogui.graph.common.const import TF_SCALING, TF_TRANSLATION


class ImageTransformsDialog(QDialog):

    def __init__(self, transforms, aspect_ratio, show_legend, parent=None):
        super().__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'transforms.ui')
        uic.loadUi(ui_path, self)

        # populate transform fields
        scale = transforms[TF_SCALING]
        offset = transforms[TF_TRANSLATION]

        self.ui_xscale.setValue(scale[0])
        self.ui_yscale.setValue(scale[1])
        self.ui_xoffset.setValue(offset[0])
        self.ui_yoffset.setValue(offset[1])

        # populate other fields
        self.ui_aspect_ratio.setCurrentIndex(aspect_ratio)
        self.ui_legend_checkbox.setChecked(show_legend)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def transforms(self):
        config = {
            "x_scale": self.ui_xscale.value(),
            "y_scale": self.ui_yscale.value(),
            "x_translate": self.ui_xoffset.value(),
            "y_translate": self.ui_yoffset.value(),
            "aspect_ratio": self.ui_aspect_ratio.currentIndex()
        }

        return config

    @property
    def show_scale(self):
        return self.ui_legend_checkbox.isChecked()

    @staticmethod
    def get(transforms, aspect_ratio, show_legend, parent=None):
        dialog = ImageTransformsDialog(transforms, aspect_ratio,
                                       show_legend, parent)
        result = dialog.exec() == QDialog.Accepted
        content = {}
        content.update(dialog.transforms)
        content["show_scale"] = dialog.show_scale

        return content, result
