from collections import OrderedDict
import os.path as op
from xml.etree.ElementTree import Element

from PyQt4 import uic
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (
    QAction, QColor, QColorDialog, QDialog, QInputDialog, QLabel
)

from karabo_gui.const import ns_karabo, OK_COLOR, ERROR_COLOR
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import String

BUTTON_COLOR_STYLESHEET = """
QPushButton#stateColor {{ background-color : rgba{}; border: none; }}
"""


class StateColorDialog(QDialog):
    def __init__(self, parent=None, items=None):
        super(StateColorDialog, self).__init__(parent)
        uiPath = op.join(op.dirname(__file__), 'displaystatecolor.ui')
        uic.loadUi(uiPath, self)

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

        self._stateMap = OrderedDict(error=ERROR_COLOR)
        self._staticText = ""

        self.widget = QLabel(parent)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumWidth(160)
        self.widget.setMinimumHeight(32)
        self.widget.setWordWrap(True)

        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.widget.setObjectName(objectName)

        colorAction = QAction("Edit State Colors...", self.widget)
        colorAction.triggered.connect(self._onChangeColors)
        textAction = QAction("Edit Static Text...", self.widget)
        textAction.triggered.connect(self._onChangeStaticText)
        self.widget.addAction(colorAction)
        self.widget.addAction(textAction)

    def setErrorState(self, isError):
        color = ERROR_COLOR if isError else OK_COLOR
        self._setColor(color)

    def valueChanged(self, box, value, timestamp=None):
        if not isinstance(box.descriptor, String):
            return  # only String types can be shown here

        if value is None:
            return

        bgColor = self._stateMap.get(value.lower(), OK_COLOR)
        self._setColor(bgColor)

        if self.widget.text() != self._staticText:
            self.widget.setText(self._staticText)

    def save(self, element):
        """ Save to a scene SVG.
        """
        element.set(ns_karabo + 'staticText', self._staticText)
        for name, color in self._stateMap.items():
            sub = Element(ns_karabo + "sc")
            sub.text = name
            red, green, blue, alpha = color
            sub.set('red', str(red))
            sub.set('green', str(green))
            sub.set('blue', str(blue))
            sub.set('alpha', str(alpha))
            element.append(sub)

    def load(self, element):
        """ Load from a scene SVG.
        """
        states = OrderedDict()
        for sub in element:
            if sub.tag != (ns_karabo + "sc"):
                continue
            key = sub.text
            red = int(sub.get('red'))
            green = int(sub.get('green'))
            blue = int(sub.get('blue'))
            alpha = int(sub.get('alpha'))
            states[key] = (red, green, blue, alpha)
        self._setStateMap(states)
        self._setStaticText(element.get(ns_karabo + 'staticText', ''))

    @pyqtSlot()
    def _onChangeColors(self):
        items = OrderedDict(self._stateMap)
        dialog = StateColorDialog(parent=self.widget, items=items)
        result = dialog.exec_()
        if result == QDialog.Accepted:
            self._setStateMap(OrderedDict(dialog.items))

        box = self.boxes[0]
        if box.hasValue():
            self.valueChanged(box, box.value)

    @pyqtSlot()
    def _onChangeStaticText(self):
        text, ok = QInputDialog.getText(self.widget, "Change static text",
                                        "Static text:")
        if ok:
            self._setStaticText(text)

    def _setColor(self, color):
        ss = self._styleSheet.format(color)
        self.widget.setStyleSheet(ss)

    def _setStateMap(self, states):
        self._stateMap = states

    def _setStaticText(self, text):
        self._staticText = text
        self.widget.setText(text)
