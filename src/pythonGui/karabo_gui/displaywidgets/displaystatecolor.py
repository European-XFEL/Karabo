from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLabel

from karabo_gui.widget import DisplayWidget
from karabo.api_2 import String

BGCOLOR_STYLESHEET = "QLabel {{ background-color : rgba{0}; }}"
ERROR_COLOR = (255, 155, 155, 128)
DEFAULT_COLOR = (225, 242, 225, 128)


class DisplayColoredState(DisplayWidget):
    category = String
    alias = "State Color Field"

    def __init__(self, box, parent):
        super(DisplayColoredState, self).__init__(box)

        self._stateMap = {
            'error': ERROR_COLOR,
        }

        self.value = None

        self.widget = QLabel(parent)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumWidth(160)
        self.widget.setMinimumHeight(32)
        self.widget.setWordWrap(True)

    def setErrorState(self, isError):
        bgColor = ERROR_COLOR if isError else DEFAULT_COLOR
        self.widget.setStyleSheet(BGCOLOR_STYLESHEET.format(bgColor))

    def valueChanged(self, box, value, timestamp=None):
        if not isinstance(box.descriptor, String):
            return  # only String types can be shown here

        if value is None:
            return

        bgColor = self._stateMap.get(value.lower(), DEFAULT_COLOR)
        self.widget.setStyleSheet(BGCOLOR_STYLESHEET.format(bgColor))

        self.value = value
        self.widget.setText(value[:30])
