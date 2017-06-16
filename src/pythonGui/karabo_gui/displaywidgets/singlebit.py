
from numpy import log2
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QInputDialog, QLabel

from karabo.common.api import State
from karabo.middlelayer import Integer
from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from .unitlabel import add_unit_label


class SingleBit(DisplayWidget):
    category = Integer
    alias = "Single Bit"

    def __init__(self, box, parent):
        super(SingleBit, self).__init__(box)
        self.bit = 0
        self.invert = False

        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setFixedSize(24, 24)
        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; "
                            "border: 2px solid black;"
                            "border-radius:12px; }} ")
        self._internal_widget.setObjectName(objectName)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

        logicAction = QAction("Invert color logic", self.widget)
        logicAction.triggered.connect(lambda: self._setInvert(not self.invert))
        changeAction = QAction("Change Bit...", self.widget)
        changeAction.triggered.connect(self._onChangeBit)
        self.widget.addAction(logicAction)
        self.widget.addAction(changeAction)

    def setErrorState(self, isError):
        color = ERROR_COLOR_ALPHA if isError else OK_COLOR
        self._setBackground(color)

    def setReadOnly(self, ro):
        self._internal_widget.setEnabled(not ro)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)
        value = (value >> self.bit) & 1 != 0
        value = not value if self.invert else value

        color = (STATE_COLORS[State.ACTIVE] if value
                 else STATE_COLORS[State.PASSIVE])
        self._setBackground(color)

    @pyqtSlot()
    def _onChangeBit(self):
        dt = self.boxes[0].descriptor.displayType
        if dt is not None and dt.startswith('bin|'):
            s, ok = QInputDialog.getItem(self.widget, 'Select Bit',
                                         'Select Bit:', dt[4:].split(','))
            if ok:
                bit = int(s.split(':')[0])
        else:
            min, max = self.boxes[0].descriptor.getMinMax()
            bit, ok = QInputDialog.getInt(self.widget, "Bit Number",
                                          "Enter number of bit:", self.bit, 0,
                                          log2(max) + 1)
        if ok:
            self._setBit(bit)

    def _setBackground(self, color):
        style = self._styleSheet.format(color)
        self._internal_widget.setStyleSheet(style)

    def _setBit(self, bit):
        """ Give derived classes a place to respond to changes. """
        self.bit = bit
        self.valueChanged(self.boxes[0], self.boxes[0].value)

    def _setInvert(self, value):
        """ Give derived classes a place to respond to changes. """
        self.invert = value
        self.valueChanged(self.boxes[0], self.boxes[0].value)
