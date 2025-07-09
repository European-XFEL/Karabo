#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 25, 2022
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
#############################################################################

from qtpy import uic
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

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
        data = dictToHash(data)
        html = create_html_hash(data, include_attributes=False)
        self.ui_text_info.setHtml(html)
