from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (QAction, QInputDialog, QLabel)

from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA, WIDGET_MIN_HEIGHT
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import Bool, String, State


class DisplayStateColor(DisplayWidget):
    category = String, Bool
    alias = "Color Field"

    def __init__(self, box, parent):
        super(DisplayStateColor, self).__init__(box)

        self._staticText = ""

        self.widget = QLabel(parent)
        objectName = generateObjectName(self)
        self.widget.setObjectName(objectName)

        textAction = QAction("Edit Static Text...", self.widget)
        textAction.triggered.connect(self._onChangeStaticText)
        self.widget.addAction(textAction)

    def typeChanged(self, box):
        desc = box.descriptor
        if isinstance(desc, String):
            self.widget.setAutoFillBackground(True)
            self.widget.setAlignment(Qt.AlignCenter)
            self.widget.setMinimumWidth(32)
            self.widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
            self.widget.setWordWrap(True)

            self._styleSheet = ("background-color: rgba{}; "
                                "border: 2px solid black;")

        elif isinstance(desc, Bool):
            self.widget.setFixedSize(24, 24)
            self._styleSheet = ("background-color: rgba{}; "
                                "border: 2px solid black;"
                                "border-radius: 12px;")

    def setErrorState(self, isError):
        color = ERROR_COLOR_ALPHA if isError else OK_COLOR
        self._setColor(color)

    def valueChanged(self, box, value, timestamp=None):
        desc = box.descriptor
        if isinstance(desc, String):
            if State(value).isDerivedFrom(State.CHANGING):
                bgColor = STATE_COLORS[State.CHANGING]
            elif State(value).isDerivedFrom(State.ACTIVE):
                bgColor = STATE_COLORS[State.ACTIVE]
            elif State(value).isDerivedFrom(State.PASSIVE):
                bgColor = STATE_COLORS[State.PASSIVE]
            elif State(value) is State.NORMAL:
                bgColor = STATE_COLORS[State.NORMAL]
            elif State(value) is State.ERROR:
                bgColor = STATE_COLORS[State.ERROR]
            else:
                bgColor = STATE_COLORS[State.UNKNOWN]
        elif isinstance(desc, Bool):
            if value:
                bgColor = STATE_COLORS[State.ACTIVE]
            else:
                bgColor = STATE_COLORS[State.PASSIVE]

        self._setColor(bgColor)

        if self.widget.text() != self._staticText:
            self.widget.setText(self._staticText)

    @pyqtSlot()
    def _onChangeStaticText(self):
        text, ok = QInputDialog.getText(self.widget, "Change static text",
                                        "Static text:")
        if ok:
            self._setStaticText(text)

    def _setColor(self, color):
        ss = self._styleSheet.format(color)
        self.widget.setStyleSheet(ss)

    def _setStaticText(self, text):
        self._staticText = text
        self.widget.setText(text)
