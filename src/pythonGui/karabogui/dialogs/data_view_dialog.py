#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 25, 2022
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from qtpy import uic
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog, QDialogButtonBox

from karabo.native import create_html_hash, dictToHash

from .utils import get_dialog_ui


class DataViewDialog(QDialog):
    def __init__(self, title, info, data, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("data_preview_dialog.ui"), self)
        self.setWindowTitle(title)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        if info is None:
            self.ui_info.setVisible(False)
        else:
            self.ui_info.setText(info)

        close_button = self.ui_buttonBox.button(QDialogButtonBox.Close)
        close_button.clicked.connect(self.accept)
        data = dictToHash(data)
        html = create_html_hash(data, include_attributes=False)
        self.ui_text_info.setHtml(html)
