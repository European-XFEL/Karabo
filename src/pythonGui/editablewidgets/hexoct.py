from util import SignalBlocker
from widget import DisplayWidget, EditableWidget

from PyQt4.QtGui import QLineEdit
from numpy import log2


class Hexadecimal(EditableWidget):
    category = "Digit"
    alias = "Hexadecimal"

    def __init__(self, box, parent):
        super(Hexadecimal, self).__init__(box)

        self.widget = QLineEdit(parent)
        self.widget.textChanged.connect(self.onEditingFinished)
        box.addWidget(self)


    @property
    def value(self):
        if self.widget.text() not in ('', '-'):
            return int(self.widget.text(), base=16)
        else:
            return 0


    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        mask = 'h' * (log2(max(abs(rmax), abs(rmin))) // 4 + 1)
        if rmin < 0:
            mask = "#" + mask
        self.widget.setInputMask(mask)


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        with SignalBlocker(self.widget):
            self.widget.setText("{:x}".format(value))


    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, self.value)
