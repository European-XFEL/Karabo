#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 21, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QHBoxLayout, QTextEdit, QWidget


class PopupWidget(QWidget):
    def __init__(self, parent=None):
        super(PopupWidget, self).__init__(parent, Qt.Tool)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)
        self.__teInfo = TextEdit(self)
        self.__teInfo.setReadOnly(True)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.__teInfo)

        self.setWindowTitle(" ")

    def setInfo(self, info):
        scrollBar = self.__teInfo.verticalScrollBar()
        pos = scrollBar.sliderPosition()

        htmlString = ("<table>" +
                      "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                              format(*p) for p in info.items()) + "</table>")
        self.__teInfo.setHtml(htmlString)

        self.__teInfo.fitHeightToContent(len(info))
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
