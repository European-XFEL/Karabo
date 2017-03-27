from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QLabel

from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import Bool, State


class DisplayColorBool(DisplayWidget):
    category = Bool
    alias = "Switch Bool"

    def __init__(self, box, parent):
        super(DisplayColorBool, self).__init__(box)
        self.invert = False
        self.widget = QLabel(parent)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setFixedSize(24, 24)
        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; "
                            "border: 2px solid black;"
                            "border-radius:12px; }} ")
        self.widget.setObjectName(objectName)

        logicAction = QAction("Invert color logic", self.widget)
        logicAction.triggered.connect(lambda: self._setInvert(not self.invert))
        self.widget.addAction(logicAction)

    def setErrorState(self, isError):
        color = ERROR_COLOR_ALPHA if isError else OK_COLOR
        self.setBackground(color)

    def valueChanged(self, box, value, timestamp=None):
        if not self.invert:
            bg_color = (STATE_COLORS[State.ACTIVE]
                        if value else STATE_COLORS[State.PASSIVE])
        else:
            bg_color = (STATE_COLORS[State.PASSIVE]
                        if value else STATE_COLORS[State.ACTIVE])

        self.setBackground(bg_color)

    def setBackground(self, color):
        style = self._styleSheet.format(color)
        self.widget.setStyleSheet(style)

    def _setInvert(self, value):
        self.invert = value
        self.valueChanged(self.boxes[0], self.boxes[0].value)