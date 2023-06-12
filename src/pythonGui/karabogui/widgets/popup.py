#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 21, 2013
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
from xml.sax.saxutils import escape

from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QPushButton, QTextEdit, QVBoxLayout, QWidget


class PopupWidget(QWidget):
    def __init__(self, can_freeze=False, parent=None):
        super().__init__(parent=parent, flags=Qt.Dialog)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)
        self.setWindowModality(Qt.NonModal)
        self._ui_info = TextEdit(self)
        self._ui_info.setReadOnly(True)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(1, 1, 1, 1)
        main_layout.addWidget(self._ui_info)

        # default is updating the pop-up
        self.can_freeze = can_freeze
        self.freeze = False
        self.info_cache = None

        # NOTE: The pop-up is used by the configurator and the navigation
        # Configurator requires freezing!
        if can_freeze:
            self._ui_freeze_button = QPushButton()
            self._ui_freeze_button.setFocusPolicy(Qt.NoFocus)
            self._ui_freeze_button.setToolTip("Freeze the pop-up information")
            self._ui_freeze_button.setStyleSheet("text-align: center")
            self._ui_freeze_button.clicked.connect(self.toggle_freeze)
            main_layout.addWidget(self._ui_freeze_button)

            # update our status!
            self.update_button_status()

        self.setWindowTitle(" ")

    def update_button_status(self):
        button_text = "Update" if self.freeze else "Freeze"
        self._ui_freeze_button.setText(button_text)

    @property
    def text(self):
        return self._ui_info.toPlainText()

    @Slot()
    def toggle_freeze(self):
        self.freeze = not self.freeze
        if self.freeze is False and self.info_cache is not None:
            self.setInfo(self.info_cache)
        self.update_button_status()

    def setInfo(self, info):
        if self.freeze:
            # We cache the lastet update for later display when the
            # freeze is toggled!
            self.info_cache = info
            return

        scrollBar = self._ui_info.verticalScrollBar()
        pos = scrollBar.sliderPosition()
        htmlString = ("<table>" +
                      "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                              format(key, escape(str(value)))
                              for key, value in info.items())
                      + "</table>")
        self._ui_info.setHtml(htmlString)

        self._ui_info.fitHeightToContent(len(info))
        # Restore scrolling position to prevent irritating scrolling up while
        # updating the popup dialog with further information on an expected
        # parameter
        scrollBar.setValue(pos)

    def reset(self):
        self.freeze = False
        self.info_cache = None
        self.update_button_status()

    def closeEvent(self, event):
        """Reimplemented function from Qt

           Erase all caching information and set the freeze to false
        """
        if self.can_freeze:
            self.reset()
        super().closeEvent(event)


class TextEdit(QTextEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._fittedWidth = 310
        self._fittedHeight = 0

    def sizeHint(self):
        sizeHint = QTextEdit.sizeHint(self)
        sizeHint.setWidth(self._fittedWidth)
        sizeHint.setHeight(self._fittedHeight)
        return sizeHint

    def fitHeightToContent(self, nbInfoKeys):
        self._fittedHeight = self.fontMetrics().height() * nbInfoKeys
        self.updateGeometry()
