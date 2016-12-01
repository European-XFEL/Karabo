import os.path as osp

from PyQt4.QtGui import QMessageBox
from PyQt4.QtSvg import QSvgWidget

from karabo.middlelayer import String

from karabo_gui.widget import DisplayWidget
from karabo_gui.schema import SlotNode


class PopUp(DisplayWidget):
    category = String
    alias = "PopUp"

    def __init__(self, box, parent):
        super(PopUp, self).__init__(None)
        self.widget = QSvgWidget(
            osp.join(osp.dirname(__file__), "speech-balloon.svg"), parent)
        self.dialog = QMessageBox(parent)
        self.dialog.setStandardButtons(QMessageBox.Ok)
        self.dialog.setModal(False)
        self.dialog.finished.connect(self.on_finished)
        self.message = box
        self.ok = self.cancel = None

    def typeChanged(self, box):
        if box is self.message:
            self.dialog.setWindowTitle(box.descriptor.displayedName)

    def addBox(self, box):
        if not isinstance(box.descriptor, SlotNode):
            return False
        elif self.ok is None:
            self.ok = box
            return True
        elif self.cancel is None:
            self.cancel = box
            self.dialog.setStandardButtons(QMessageBox.Ok | QMessageBox.Cancel)
            return True
        return False

    def on_finished(self, result):
        if result == QMessageBox.Ok and self.ok is not None:
            self.ok.execute()
        elif result == QMessageBox.Cancel and self.cancel is not None:
            self.cancel.execute()

    @property
    def boxes(self):
        ret = [self.message]
        if self.ok is not None:
            ret.append(self.ok)
        if self.cancel is not None:
            ret.append(self.cancel)
        return ret

    def valueChanged(self, box, value, timestamp=None):
        if box is not self.message:
            return
        self.dialog.setText(value)
        self.dialog.setVisible(value != "")
