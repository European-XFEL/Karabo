from __future__ import absolute_import
import icons
from widget import DisplayWidget

from karabo import hashtypes

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QApplication, QDialog, QFileDialog, QLabel,
                         QPixmap)

from os import path
import re


class Label(QLabel):
    imageChanged = pyqtSignal(str, object)


    def __init__(self, parent):
        QLabel.__init__(self, parent)
        self.setAcceptDrops(True)
        self.setPixmap(None)


    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat('text/uri-list'):
            event.acceptProposedAction()


    def dropEvent(self, event):
        name = event.mimeData().urls()[0].toLocalFile()
        self.setFilename(name)


    def setFilename(self, name):
        pixmap = QPixmap(name)
        self.setPixmap(pixmap)
        self.imageChanged.emit(name, pixmap)


    def setPixmap(self, pixmap):
        if pixmap is None:
            QLabel.setPixmap(self, icons.no.pixmap(100))
        else:
            QLabel.setPixmap(self, pixmap)


class Icons(DisplayWidget):
    def __init__(self, box, parent):
        self.widget = QLabel(parent)
        action = QAction("Change Icons...", self.widget)
        self.widget.addAction(action)

        self.dialog = QDialog()
        action.triggered.connect(self.dialog.exec_)
        uic.loadUi(path.join(path.dirname(__file__), 'icons.ui'), self.dialog)
        self.dialog.deleteValue.clicked.connect(self.on_deleteValue_clicked)
        self.dialog.addValue.clicked.connect(self.on_addValue_clicked)
        self.dialog.list.currentRowChanged.connect(
            self.on_list_currentRowChanged)
        self.dialog.image.imageChanged.connect(self.on_image_imageChanged)
        self.dialog.open.clicked.connect(self.on_open_clicked)
        self.dialog.paste.clicked.connect(self.on_paste_clicked)
        self.dialog.finished.connect(self.on_dialog_finished)
        super(Icons, self).__init__(box)


    def on_list_currentRowChanged(self, row):
        self.dialog.image.setPixmap(self.list[row][-1])


    def on_image_imageChanged(self, name, pixmap):
        cr = self.dialog.list.currentRow()
        self.list[cr] = self.list[cr][:-2] + (name, pixmap)


    def on_open_clicked(self):
        name = QFileDialog.getOpenFileName(self.dialog, "Open Icon")
        if name:
            self.dialog.image.setFilename(name)


    def on_dialog_finished(self):
        self.valueChanged(self.boxes[0], self.boxes[0].value)


    def on_paste_clicked(self):
        mime = QApplication.clipboard().mimeData()
        name = mime.urls()[0].toLocalFile()
        self.dialog.image.setFilename(name)


    def on_deleteValue_clicked(self):
        cr = self.dialog.list.currentRow()
        if self.list[cr][0] is not None:
            del self.list[cr]
            self.dialog.list.takeItem(cr)

    def setPixmap(self, p):
        if p is None:
            self.widget.setPixmap(icons.no.pixmap(100))
        else:
            self.widget.setPixmap(p)


class TextIcons(Icons):
    category = "State"
    alias = "Text Icons"


    def typeChanged(self, box):
        self.list = [(None, None, None)]
        self.dialog.stack.setCurrentWidget(self.dialog.textPage)
        self.dialog.up.clicked.connect(self.on_up_clicked)
        self.dialog.down.clicked.connect(self.on_down_clicked)


    def on_up_clicked(self):
        l = self.dialog.list
        cr = l.currentRow()
        if not 0 < cr < len(self.list) - 1:
            return
        l.insertItem(cr - 1, l.takeItem(cr))
        self.list[cr - 1], self.list[cr] = self.list[cr], self.list[cr - 1]


    def on_down_clicked(self):
        l = self.dialog.list
        cr = l.currentRow()
        if cr >= len(self.list) - 2:
            return
        l.insertItem(cr + 1, l.takeItem(cr))
        self.list[cr + 1], self.list[cr] = self.list[cr], self.list[cr + 1]


    def on_addValue_clicked(self):
        self.list.insert(0, (re.compile(self.dialog.textValue.text()),
                             None, None))
        self.dialog.list.insertItem(0, self.dialog.textValue.text())


    def valueChanged(self, box, value, timestamp=None):
        for r, i, p in self.list:
            if r is None or r.match(value):
                self.setPixmap(p)
                return


class DigitIcons(Icons):
    category = "Digit"
    alias = "Digit Icons"

    def on_addValue_clicked(self):
        equal = self.dialog.lessEqual.isChecked()
        for i, (v, f, _, _,) in enumerate(self.list):
            if self.value.value() < v or f and self.value.value() == v:
                break
        self.list.insert(i, (self.value.value(), equal, None, None))
        self.dialog.list.insertItem(i, "{} {}".format("<=" if equal else "<",
                                                      self.value.value()))


    def typeChanged(self, box):
        self.list = [(None, None, None, None)]
        if isinstance(box.descriptor, hashtypes.Integer):
            self.dialog.stack.setCurrentWidget(self.dialog.intPage)
            self.value = self.dialog.intValue
        else:
            self.dialog.stack.setCurrentWidget(self.dialog.floatPage)
            self.value = self.dialog.floatValue
        min, max = box.descriptor.getMinMax()
        if min is not None:
            self.value.setMinimum(min)
        if max is not None:
            self.value.setMaximum(max)


    def valueChanged(self, box, value, timestamp=None):
        for v, f, i, p in self.list:
            if value < v or value == v and f or v is None:
                self.setPixmap(p)
                break
