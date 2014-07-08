from __future__ import absolute_import
from const import ns_karabo
import icons
from widget import DisplayWidget

from karabo import hashtypes

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, QByteArray, QBuffer
from PyQt4.QtGui import (QAction, QApplication, QDialog, QFileDialog, QLabel,
                         QPixmap)

from os import path
import urllib
import re
from xml.etree.ElementTree import Element


class Label(QLabel):
    newMime = pyqtSignal(str)


    def __init__(self, parent):
        QLabel.__init__(self, parent)
        self.setAcceptDrops(True)
        self.setPixmap(None)


    def dragEnterEvent(self, event):
        event.acceptProposedAction()


    def dropEvent(self, event):
        self.newMime.emit(mime.mimeData())


    def setPixmap(self, pixmap):
        if pixmap is None:
            QLabel.setPixmap(self, icons.no.pixmap(100))
        else:
            QLabel.setPixmap(self, pixmap)


class Icons(DisplayWidget):
    def __init__(self, box, parent):
        super(Icons, self).__init__(box)
        
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
        self.dialog.image.newMime.connect(self.setMime)
        self.dialog.open.clicked.connect(self.on_open_clicked)
        self.dialog.paste.clicked.connect(self.on_paste_clicked)
        self.dialog.copy.clicked.connect(self.on_copy_clicked)
        self.dialog.finished.connect(self.on_dialog_finished)

        self.list = [(None, None, None, None)]


    def on_list_currentRowChanged(self, row):
        self.dialog.image.setPixmap(self.list[row][-1])


    def setMime(self, mime):
        if mime.hasImage():
            ba = QByteArray()
            try:
                buffer = QBuffer(ba)
                buffer.open(QBuffer.WriteOnly)
                image = mime.imageData().save(buffer, "PNG")
                url = self.project.addResource("icon", buffer.data())
            finally:
                buffer.close()
        else:
            try:
                url = mime.text().split()[0].strip()
            except Exception as e:
                e.message = "Could not open URL or Image"
                raise
        self.setURL(url)


    def setURL(self, url):
        cr = self.dialog.list.currentRow()
        pixmap = self.getPixmap(url)
        self.list[cr] = self.list[cr][:-2] + (url, pixmap)
        self.dialog.image.setPixmap(pixmap)


    def on_open_clicked(self):
        name = QFileDialog.getOpenFileName(self.dialog, "Open Icon")
        if name:
            url = "file://" + urllib.pathname2url(name)
            self.dialog.image.setURL(url)


    def on_dialog_finished(self, result):
        if result == QDialog.Accepted:
            self.valueChanged(self.boxes[0], self.boxes[0].value)


    def on_paste_clicked(self):
        self.setMime(QApplication.clipboard().mimeData())


    def on_copy_clicked(self):
        cr = self.dialog.list.currentRow()
        url = self.project.addResource("icon",
                                       self.project.getURL(self.list[cr][2]))
        self.setURL(url)


    def on_deleteValue_clicked(self):
        cr = self.dialog.list.currentRow()
        if self.list[cr][0] is not None:
            self.dialog.list.takeItem(cr)
            del self.list[cr]


    def setPixmap(self, p):
        if p is None:
            self.widget.setPixmap(icons.no.pixmap(100))
        else:
            self.widget.setPixmap(p)


    def getPixmap(self, url):
        pixmap = QPixmap()
        if not pixmap.loadFromData(self.project.getURL(url)):
            raise RuntimeError("could not read image from url {}".format(url))
        return pixmap


    def loadImage(self, e):
        url = e.get("image")
        if url is None:
            return None, None
        else:
            return url, self.getPixmap(url)


    def setItems(self, items):
        self.dialog.list.clear()
        self.dialog.list.addItems(list(items))
        self.dialog.list.setCurrentRow(0)


class TextIcons(Icons):
    category = "State"
    alias = "Text Icons"


    def __init__(self, box, parent):
        super(TextIcons, self).__init__(box, parent)
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
                             self.dialog.textValue.text(),
                             None, None))
        self.dialog.list.insertItem(0, self.dialog.textValue.text())


    def valueChanged(self, box, value, timestamp=None):
        for r, _, _, p in self.list:
            if r is None or r.match(value):
                self.setPixmap(p)
                return


    def save(self, e):
        for _, t, i, _ in self.list:
            ee = Element(ns_karabo + "re")
            if t is not None:
                ee.text = t
            if i is not None:
                ee.set('image', i)
            e.append(ee)


    def load(self, e):
        self.list = [((re.compile(ee.text), ee.text)
                       if ee.text else (None, None)) +
                     self.loadImage(ee) for ee in e]
        self.setItems("default" if t is None else t
                      for _, t, _, _ in self.list)


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


    def save(self, e):
        for v, f, i, _ in self.list:
            ee = Element(ns_karabo + "value")
            if v is not None:
                ee.text = repr(v)
            if f is not None:
                ee.set('equal', unicode(f).lower())
            if i is not None:
                ee.set('image', i)
            e.append(ee)


    def load(self, e):
        parse = int if isinstance(self.boxes[0].descriptor,
                                  hashtypes.Integer) else float
        self.list = [((parse(ee.text), ee.get('equal') == 'true')
                       if ee.get('equal') else (None, None)) +
                     self.loadImage(ee) for ee in e]
        self.setItems("default" if v is None
                      else "{} {}".format("<=" if f else "<", v)
                      for v, f, _, _ in self.list)
