
from numpy import log2
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QCheckBox, QInputDialog

from karabo.middlelayer import Integer
from karabo_gui.widget import DisplayWidget


class SingleBit(DisplayWidget):
    category = Integer
    alias = "Single Bit"

    def __init__(self, box, parent):
        super(SingleBit, self).__init__(box)
        self.widget = QCheckBox(parent)
        action = QAction("Change Bit...", self.widget)
        action.triggered.connect(self.onChangeBit)
        self.widget.addAction(action)
        self.bit = 0

    def setReadOnly(self, ro):
        self.widget.setEnabled(not ro)

    @pyqtSlot()
    def onChangeBit(self):
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
            self.valueChanged(self.boxes[0], self.boxes[0].value)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.setChecked((value >> self.bit) & 1 != 0)

    def _setBit(self, bit):
        """ Give derived classes a place to respond to changes. """
        self.bit = bit
