from widget import DisplayWidget

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QCheckBox, QInputDialog

class SingleBit(DisplayWidget):
    category = "Digit"
    alias = "Single Bit"


    def __init__(self, box, parent):
        self.widget = QCheckBox(parent)
        action = QAction("Change Bit...", self.widget)
        action.triggered.connect(self.onChangeBit)
        self.widget.addAction(action)
        self.bit = 0
        super(SingleBit, self).__init__(box)


    def setReadOnly(self, ro):
        self.widget.setEnabled(ro)


    @pyqtSlot()
    def onChangeBit(self):
        bit, ok = QInputDialog.getInt(self.widget, "Bit Number",
                                      "Enter number of bit:", min=0, max=64)
        if ok:
            self.bit = bit
            self.valueChanged(self.boxes[0], self.boxes[0].value)


    def valueChanged(self, box, value, timestamp=None):
        self.widget.setChecked((value >> self.bit) & 1 != 0)
