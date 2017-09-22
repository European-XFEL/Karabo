from PyQt4.QtCore import QSize, Qt, pyqtSignal
from PyQt4.QtGui import QWidget, QPainter

from karabo.middlelayer import Integer
from karabo_gui.widget import DisplayWidget, EditableWidget
from .unitlabel import add_unit_label


class BitfieldWidget(QWidget):
    valueChanged = pyqtSignal(int)

    def __init__(self, parent):
        QWidget.__init__(self, parent)
        self.size = 16
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


class Bitfield(EditableWidget, DisplayWidget):
    category = Integer
    alias = "Bit Field"

    def __init__(self, box, parent):
        super(Bitfield, self).__init__(box)
        self._internal_widget = BitfieldWidget(parent)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

    @property
    def editWidget(self):
        return self._internal_widget

    def typeChanged(self, box):
        self._internal_widget.size = box.descriptor.numpy().nbytes * 8
        self.widget.update()

    def setReadOnly(self, ro):
        self._internal_widget.readonly = ro
        if not ro:
            self._internal_widget.valueChanged.connect(self.onEditingFinished)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    @property
    def value(self):
        return self._internal_widget.value

    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0
        self._internal_widget.value = value
        self.widget.updateLabel(box)
        self.widget.update()
