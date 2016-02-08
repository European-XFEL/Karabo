import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (
    QAction, QColor, QColorDialog, QDialog, QInputDialog, QLabel
)

from karabo_gui.widget import DisplayWidget
from karabo.api_2 import String

LABEL_COLOR_STYLESHEET = "QLabel#stateColor {{ background-color : rgba{}; }}"
BUTTON_COLOR_STYLESHEET = """
QPushButton#stateColor {{ background-color : rgba{}; border: none; }}
"""
OBJECT_NAME = "stateColor"
ERROR_COLOR = (255, 155, 155, 128)
DEFAULT_COLOR = (225, 242, 225, 128)


class StateColorDialog(QDialog):
    def __init__(self, parent=None, items=None):
        super(StateColorDialog, self).__init__(parent)
        uiPath = op.join(op.dirname(__file__), 'displaystatecolor.ui')
        uic.loadUi(uiPath, self)

        self.stateColor.setObjectName(OBJECT_NAME)
        self.stateList.addItems([name.capitalize() for name in items])
        self.items = items

    @pyqtSlot()
    def on_addState_clicked(self):
        text, ok = QInputDialog.getText(self, "Add a state", "")
        if ok:
            sanitizedName = text.lower()
            self.stateList.addItem(sanitizedName.capitalize())
            self.items[sanitizedName] = (0, 0, 0, 255)

    @pyqtSlot()
    def on_removeState_clicked(self):
        key = self._getSelectedKey()
        self.stateList.takeItem(self.stateList.currentRow())
        del self.items[key]

    @pyqtSlot()
    def on_stateColor_clicked(self):
        color = self.items[self._getSelectedKey()]
        color = QColorDialog.getColor(initial=QColor(*color), parent=self)
        if color.isValid():
            colorTuple = color.getRgb()
            self.items[self._getSelectedKey()] = colorTuple
            self._setStateButtonColor(colorTuple)

    @pyqtSlot(int)
    def on_stateList_currentRowChanged(self, index):
        color = self.items[self._getSelectedKey()]
        self._setStateButtonColor(color)

    def _getSelectedKey(self):
        item = self.stateList.currentItem()
        return item.text().lower()

    def _setStateButtonColor(self, color):
        styleSheet = BUTTON_COLOR_STYLESHEET.format(color)
        self.stateColor.setStyleSheet(styleSheet)


class DisplayStateColor(DisplayWidget):
    category = String
    alias = "State Color Field"

    def __init__(self, box, parent):
        super(DisplayStateColor, self).__init__(box)

        self._stateMap = {
            'error': ERROR_COLOR,
        }

        self.value = None

        self.widget = QLabel(parent)
        self.widget.setObjectName(OBJECT_NAME)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumWidth(160)
        self.widget.setMinimumHeight(32)
        self.widget.setWordWrap(True)

        action = QAction("Edit State Colors...", self.widget)
        action.triggered.connect(self.onChangeColors)
        self.widget.addAction(action)

    def setErrorState(self, isError):
        bgColor = ERROR_COLOR if isError else DEFAULT_COLOR
        self._setColor(bgColor)

    def valueChanged(self, box, value, timestamp=None):
        if not isinstance(box.descriptor, String):
            return  # only String types can be shown here

        if value is None:
            return

        bgColor = self._stateMap.get(value.lower(), DEFAULT_COLOR)
        self._setColor(bgColor)

        self.value = value
        self.widget.setText(value[:30])

    @pyqtSlot()
    def onChangeColors(self):
        dialog = StateColorDialog(parent=self.widget, items=self._stateMap)
        result = dialog.exec_()
        if result == QDialog.Accepted:
            self._stateMap = dialog.items

        box = self.boxes[0]
        if box.hasValue():
            self.valueChanged(box, box.value)

    def _setColor(self, color):
        styleSheet = LABEL_COLOR_STYLESHEET.format(color)
        self.widget.setStyleSheet(styleSheet)