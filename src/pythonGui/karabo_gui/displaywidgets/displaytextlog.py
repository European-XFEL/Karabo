#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import time

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QFrame, QHBoxLayout, QPushButton, QVBoxLayout, QTextEdit)

from karabo.middlelayer import String
import karabo_gui.icons as icons
from karabo_gui.const import FINE_COLOR
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget

W_SIZE = 32


class DisplayTextLog(DisplayWidget):
    category = String
    alias = "Text Log"

    def __init__(self, box, parent):
        super(DisplayTextLog, self).__init__(box)

        self.widget = QFrame(parent)
        ver_layout = QVBoxLayout(self.widget)

        self.log_widget = QTextEdit()
        # bar is required to move the slider after value set
        self._bar = self.log_widget.verticalScrollBar()

        # no focus for this widget!
        self.log_widget.setFocusPolicy(Qt.NoFocus)
        ver_layout.addWidget(self.log_widget)

        # start horizontal layout with button and spacer
        hor_layout = QHBoxLayout()

        # strech is required to move the button to the right
        hor_layout.addStretch(1)

        button = QPushButton()
        button.setFixedSize(W_SIZE, W_SIZE)
        button.setFocusPolicy(Qt.NoFocus)
        button.setToolTip("Clear log.")
        button.setIcon(icons.editClear)
        button.clicked.connect(self._clear_log)
        hor_layout.addWidget(button)

        # spacer item to align button to the right!

        ver_layout.addLayout(hor_layout)

        # nice color background
        objectName = generateObjectName(self)
        sheet = ("QWidget#{} {{ background-color : rgba{}; }}"
                 "".format(objectName, FINE_COLOR))
        self.log_widget.setObjectName(objectName)
        self.log_widget.setStyleSheet(sheet)

    @pyqtSlot()
    def _clear_log(self):
        self.log_widget.clear()

    def _write_log(self, text):
        # XXX: Real timestamp used in the future when John killed the box!
        full_time = time.localtime()
        stamp = "[{:02d}:{:02d}:{:02d}]".format(full_time.tm_hour,
                                                full_time.tm_min,
                                                full_time.tm_sec)
        TEMPLATE = "{}: {}"

        item = TEMPLATE.format(stamp, text)
        self.log_widget.append(item)
        # update our bar
        self._bar.setValue(self._bar.maximum())

    def valueChanged(self, box, value, timestamp=None):
        # catch both None and empty strings
        if not value:
            return

        self._write_log(value)
