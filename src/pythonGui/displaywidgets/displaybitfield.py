from __future__ import division
import globals

from widget import DisplayWidget, EditableWidget

from karabo.karathon import Types

from PyQt4.QtCore import QSize, Qt, pyqtSignal
from PyQt4.QtGui import QWidget, QPainter

class Bitfield(QWidget):
    valueChanged = pyqtSignal(int)


    def __init__(self, type):
        QWidget.__init__(self)
        self.size = {Types.INT8: 8, Types.UINT8: 8, Types.INT16: 16,
                     Types.UINT16: 16, Types.INT32: 32, Types.UINT32: 32,
                     Types.INT64: 64, Types.UINT64: 64}[type]
        self.value = 0
        self.readonly = False


    def sizeHint(self):
        return QSize(self.size * 5 // 4, 20)


    def mousePressEvent(self, event):
        if self.readonly:
            return

        w, h = self._getwh()
        bit = (event.pos().y() // h) + 4 * (event.pos().x() // w)
        self.value ^= 1 << bit
        self.valueChanged.emit(self.value)
        self.update()


    def _getwh(self):
        w = self.geometry().width() * 4 // self.size
        h = self.geometry().height() // 4
        return w, h


    def paintEvent(self, event):
        w, h = self._getwh()
        p = QPainter(self)
        try:
            p.fillRect(0, 0, w * self.size // 4 + 1, h * 4 + 1, Qt.gray)
            for pos in range(self.size):
                color = Qt.white if self.value & 1 << pos > 0 else Qt.black
                p.fillRect(pos // 4 * w + 1, pos % 4 * h + 1, w - 1, h - 1, color)
        finally:
            p.end()


    def setValue(self, value):
        self.value = value
        self.update()


class EditableBitfield(EditableWidget):
    category = "Digit"
    alias = "Bit Field"

    def __init__(self, value, valueType, **params):
        super(EditableBitfield, self).__init__(**params)

        self.widget = Bitfield(valueType)
        self.widget.value = value
        self.widget.valueChanged.connect(self.onEditingFinished)


    @property
    def value(self):
        return self.widget.value


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0
        self.widget.value = value
        self.widget.update()


    def onEditingFinished(self, value):
        self.valueEditingFinished(self.keys[0], value)


class DisplayBitfield(DisplayWidget):
    category = "Digit"
    alias = "Bit Field"

    def __init__(self, valueType, **params):
        super(DisplayBitfield, self).__init__(**params)

        self.widget = Bitfield(valueType)
        self.widget.readonly = True


    @property
    def value(self):
        return self.widget.value


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return

        if value != self.value:
            self.widget.setValue(value)
