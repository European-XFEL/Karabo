import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot
from PyQt5.QtWidgets import QDialog

from karabo.common.scenemodel.const import SCENE_FONT_SIZES, SCENE_FONT_WEIGHTS


class FormatLabelDialog(QDialog):

    def __init__(self, font_size=10, font_weight="normal", parent=None):
        super(FormatLabelDialog, self).__init__(parent=parent)
        uic.loadUi(op.join(op.dirname(__file__), "format_label.ui"), self)

        # Populate font size combobox and set current index
        sizes_string = [str(size) for size in SCENE_FONT_SIZES]
        self.font_size_combobox.addItems(sizes_string)
        index = SCENE_FONT_SIZES.index(font_size)
        self.font_size_combobox.setCurrentIndex(index)

        # Populate font weight combobox and set current index
        self.font_weight_combobox.addItems(SCENE_FONT_WEIGHTS)
        index = SCENE_FONT_WEIGHTS.index(font_weight)
        self.font_weight_combobox.setCurrentIndex(index)

        # Add triggers to change preview
        self.font_size_combobox.currentIndexChanged.connect(self._preview)
        self.font_weight_combobox.currentIndexChanged.connect(self._preview)

        # Update preview to reflect current format
        self._preview()

    @pyqtSlot()
    def _preview(self):
        font = self.preview_label.font()
        font.setPointSize(self.font_size)
        font.setBold(self.font_weight == "bold")
        self.preview_label.setFont(font)

    @property
    def font_size(self):
        return int(self.font_size_combobox.currentText())

    @property
    def font_weight(self):
        return self.font_weight_combobox.currentText()