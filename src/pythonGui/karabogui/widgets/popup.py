#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 21, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QPushButton, QTextEdit, QVBoxLayout, QWidget


class PopupWidget(QWidget):
    def __init__(self, can_freeze=False, parent=None):
        super(PopupWidget, self).__init__(parent, Qt.Tool)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)
        self._ui_info = TextEdit(self)
        self._ui_info.setReadOnly(True)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(1, 1, 1, 1)
        main_layout.addWidget(self._ui_info)

        # default is updating the pop-up
        self.freeze = False

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

    @pyqtSlot()
    def toggle_freeze(self):
        self.freeze = not self.freeze
        self.update_button_status()

    def setInfo(self, info):
        if self.freeze:
            return

        scrollBar = self._ui_info.verticalScrollBar()
        pos = scrollBar.sliderPosition()
        htmlString = ("<table>" +
                      "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                              format(*p) for p in info.items()) + "</table>")
        self._ui_info.setHtml(htmlString)

        self._ui_info.fitHeightToContent(len(info))
        # Restore scrolling position to prevent irritating scrolling up while
        # updating the popup dialog with further information on an expected
        # parameter
        scrollBar.setValue(pos)


class TextEdit(QTextEdit):
    def __init__(self, parent=None):
        super(TextEdit, self).__init__(parent)
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
