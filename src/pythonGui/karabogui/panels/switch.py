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

from qtpy.QtCore import QPropertyAnimation, QRect, Qt, Signal, Slot
from qtpy.QtWidgets import QFrame, QHBoxLayout, QLabel, QPushButton, QWidget

SWITCH_NORMAL = "NORMAL"
SWITCH_EXPERT = "EXPERT"

BUTTON_WIDTH = 22
FRAME_WIDTH = 50
FRAME_HEIGHT = 22
BUTTON_HEIGHT = 18
CENTER = (FRAME_HEIGHT - BUTTON_HEIGHT) // 2

BACKGROUND = "gray"
SWITCH_COLOR = "lightgreen"


class ClickFrame(QFrame):
    clicked = Signal()

    def mousePressEvent(self, event):
        self.clicked.emit()
        super().mousePressEvent(event)


class SwitchButton(QWidget):
    toggled = Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)

        self.layout = QHBoxLayout(self)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(5)

        self.switch_frame = ClickFrame(self)
        self.switch_frame.setFixedSize(FRAME_WIDTH, FRAME_HEIGHT)
        self.switch_frame.setStyleSheet(
            f"background-color: {BACKGROUND};"
            "border-radius: 9px;"
        )
        self.button = QPushButton(self.switch_frame)
        self.button.setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT)
        self.button.setStyleSheet(
            "background-color: white;"
            "border-radius: 9px;"
            f"border: 2px solid {BACKGROUND};"
        )
        self.button.move(2, CENTER)

        self.label = QLabel(SWITCH_NORMAL)
        self.label.setFixedSize(80, FRAME_HEIGHT)
        self.label.setStyleSheet(
            "font-size: 12px; "
            "font-weight: bold;")
        self.label.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)

        self.animation = QPropertyAnimation(
            targetObject=self.button, propertyName=b"geometry")
        self.animation.setDuration(200)
        self.button.clicked.connect(self.toggle_switch)
        self.switch_frame.clicked.connect(self.toggle_switch)

        self.layout.addWidget(self.switch_frame)
        self.layout.addWidget(self.label)
        self.setLayout(self.layout)

        self.is_switched = False

    @Slot()
    def toggle_switch(self):
        self.animation.stop()
        if self.is_switched:
            self.animation.setStartValue(
                QRect(FRAME_WIDTH - BUTTON_WIDTH - 2, CENTER, BUTTON_WIDTH,
                      BUTTON_HEIGHT))
            self.animation.setEndValue(
                QRect(2, CENTER, BUTTON_WIDTH - 2, BUTTON_HEIGHT))
            self.switch_frame.setStyleSheet(
                f"background-color: {BACKGROUND};"
                "border-radius: 9px;")
            self.label.setText(SWITCH_NORMAL)
        else:
            self.animation.setStartValue(
                QRect(2, CENTER, BUTTON_WIDTH, BUTTON_HEIGHT))
            self.animation.setEndValue(
                QRect(FRAME_WIDTH - BUTTON_WIDTH - 2, CENTER, BUTTON_WIDTH - 2,
                      BUTTON_HEIGHT))
            self.switch_frame.setStyleSheet(
                f"background-color: {SWITCH_COLOR};"
                "border-radius: 9px;")
            self.label.setText(SWITCH_EXPERT)

        self.is_switched = not self.is_switched
        self.toggled.emit(self.is_switched)
        self.animation.start()
