import os

from PyQt4 import uic
from PyQt4.QtGui import QDialog

from karabogui.graph.common.const import SCALING, TRANSLATION


class ImageTransformsDialog(QDialog):

    def __init__(self, transforms, aspect_ratio, show_legend, parent=None):
        super(ImageTransformsDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'image_transforms.ui')
        uic.loadUi(ui_path, self)

        # populate transform fields
        scale = transforms[SCALING]
        offset = transforms[TRANSLATION]

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
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.transforms)
        content["show_scale"] = dialog.show_scale

        return content, result
