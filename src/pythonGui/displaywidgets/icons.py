
from const import ns_karabo
import icons
from widget import DisplayWidget

from karabo import hashtypes

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QByteArray, QBuffer
from PyQt4.QtGui import (QAction, QApplication, QDialog, QFileDialog, QLabel,
                         QMessageBox, QPixmap)

from os import path
import urllib.request
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


class Item:
    value = None
    url = None
    pixmap = None

    def __init__(self, element=None, project=None):
        if element is None:
            return
        url = element.get("image")
        if url is not None:
            self.url = url
            self.getPixmap(project)

    def getPixmap(self, project):
        pixmap = QPixmap()
        try:
            if not pixmap.loadFromData(project.getURL(self.url)):
                raise RuntimeError("could not read image from url {}".
                                   format(self.url))
        except KeyError:
            QMessageBox.warning(None, "Icon not found",
                                'could not find an icon')
            self.pixmap = None
        self.pixmap = pixmap


class Dialog(QDialog):
    def __init__(self, project, items):
        super().__init__()
        self.project = project
        self.items = items
        uic.loadUi(path.join(path.dirname(__file__), 'icons.ui'), self)
        self.list.addItems([self.textFromItem(item) for item in items])

    @pyqtSlot(int)
    def on_list_currentRowChanged(self, row):
        self.image.setPixmap(self.items[row].pixmap)

    def setURL(self, url):
        if url.startswith("file:"):
            url = self.project.addResource("icon", self.project.getURL(url))
        item = self.items[self.list.currentRow()]
        item.url = url
        item.getPixmap(self.project)
        self.image.setPixmap(item.pixmap)
        self.copy.setEnabled(not url.startswith("project:"))

    def on_image_newMime(self, mime):
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

    @pyqtSlot()
    def on_open_clicked(self):
        name = QFileDialog.getOpenFileName(self, "Open Icon")
        if name:
            url = "file://" + urllib.request.pathname2url(name)
            self.setURL(url)

    @pyqtSlot()
    def on_paste_clicked(self):
        self.on_image_newMime(QApplication.clipboard().mimeData())

    @pyqtSlot()
    def on_copy_clicked(self):
        cr = self.list.currentRow()
        url = self.project.addResource("icon",
                                       self.project.getURL(self.items[cr].url))
        self.setURL(url)

    @pyqtSlot()
    def on_deleteValue_clicked(self):
        cr = self.list.currentRow()
        if self.items[cr].value is not None:
            self.list.takeItem(cr)
            del self.items[cr]

    @pyqtSlot()
    def on_up_clicked(self):
        cr = self.list.currentRow()
        if not 0 < cr < len(self.items) - 1:
            return
        self.list.insertItem(cr - 1, self.list.takeItem(cr))
        self.items[cr - 1], self.items[cr] = self.items[cr], self.items[cr - 1]

    @pyqtSlot()
    def on_down_clicked(self):
        cr = self.list.currentRow()
        if cr >= len(self.items) - 2:
            return
        self.list.insertItem(cr + 1, self.list.takeItem(cr))
        self.items[cr + 1], self.items[cr] = self.items[cr], self.items[cr + 1]

    def exec(self):
        super().exec()
        return self.items


class Icons(DisplayWidget):
    def __init__(self, box, parent):
        super(Icons, self).__init__(box)

        self.widget = QLabel(parent)
        action = QAction("Change Icons...", self.widget)
        self.widget.addAction(action)
        action.triggered.connect(self.showDialog)
        self.items = [Item()]

    @classmethod
    def isCompatible(self, box, ro):
        return super().isCompatible(box, ro) and box.descriptor.options is None

    def showDialog(self):
        box = self.boxes[0]
        dialog = self.Dialog(self.project, self.items, box.descriptor)
        self.items = dialog.exec()
        self.valueChanged(box, box.value)

    def setPixmap(self, p):
        if p is None:
            self.widget.setPixmap(icons.no.pixmap(100))
        else:
            self.widget.setPixmap(p)


class TextDialog(Dialog):
    def __init__(self, project, items, descriptor):
        super().__init__(project, items)
        self.stack.setCurrentWidget(self.textPage)

    @pyqtSlot()
    def on_addValue_clicked(self):
        item = Item()
        item.value = self.textValue.text()
        item.re = re.compile(item.value)
        self.items.insert(0, item)
        self.list.insertItem(0, self.textValue.text())

    def textFromItem(self, item):
        if item.value is None:
            return "default"
        return item.value


class TextIcons(Icons):
    alias = "Icons"
    category = hashtypes.String
    Dialog = TextDialog

    def valueChanged(self, box, value, timestamp=None):
        for item in self.items:
            if item.value is None or item.re.match(value):
                self.setPixmap(item.pixmap)
                return

    def save(self, e):
        for item in self.items:
            ee = Element(ns_karabo + "re")
            if item.value is not None:
                ee.text = item.value
            if item.url is not None:
                ee.set('image', item.url)
            e.append(ee)

    def load(self, e):
        self.items = []
        for ee in e:
            item = Item(ee, self.project)
            if ee.text:
                item.value = ee.text
                item.re = re.compile(item.value)
            self.items.append(item)


class DigitDialog(Dialog):
    def __init__(self, project, items, descriptor):
        super().__init__(project, items)
        if isinstance(descriptor, hashtypes.Integer):
            self.value = self.intValue
            self.stack.setCurrentWidget(self.intPage)
        else:
            self.value = self.floatValue
            self.stack.setCurrentWidget(self.floatPage)
        min, max = descriptor.getMinMax()
        if min is not None:
            self.value.setMinimum(min)
        if max is not None:
            self.value.setMaximum(max)


    @pyqtSlot()
    def on_addValue_clicked(self):
        for i, item in enumerate(self.items):
            if (item.value is None or self.value.value() < item.value or
                    self.value.value() == item.value and item.equal):
                break
        item = Item()
        item.value = self.value.value()
        item.equal = self.lessEqual.isChecked()
        self.items.insert(i, item)
        self.list.insertItem(i, self.textFromItem(item))

    def textFromItem(self, item):
        if item.value is None:
            return "default"
        return "{} {}".format("<=" if item.equal else "<", item.value)


class DigitIcons(Icons):
    alias = "Icons"
    category = hashtypes.Integer, hashtypes.Number
    Dialog = DigitDialog

    def valueChanged(self, box, value, timestamp=None):
        for item in self.items:
            if (item.value is None or value < item.value or
                    value == item.value and item.equal):
                self.setPixmap(item.pixmap)
                break

    def save(self, e):
        for item in self.items:
            ee = Element(ns_karabo + "value")
            if item.value is not None:
                ee.text = repr(item.value)
                if item.equal is not None:
                    ee.set('equal', str(item.equal).lower())
            if item.url is not None:
                ee.set('image', item.url)
            e.append(ee)

    def load(self, e):
        if isinstance(self.boxes[0].descriptor, hashtypes.Integer):
            parse = int
        else:
            parse = float
        self.items = []
        for ee in e:
            item = Item(ee, self.project)
            if ee.get('equal'):
                item.value = parse(ee.text)
                item.equal = ee.get('equal') == 'true'
            self.items.append(item)


class SelectionDialog(Dialog):
    def __init__(self, project, items, descriptor):
        super().__init__(project, items)
        self.editable.hide()

    def textFromItem(self, item):
        return item.value


class SelectionIcons(Icons):
    alias = "Selection Icons"
    Dialog = SelectionDialog

    @classmethod
    def isCompatible(self, box, ro):
        return ro and box.descriptor.options is not None

    def typeChanged(self, box):
        items = []
        for o in box.descriptor.options:
            for item in self.items:
                if item.value == o:
                    items.append(item)
            else:
                item = Item()
                item.value = o
                print('added item', o)
                items.append(item)
        self.items = items

    def valueChanged(self, box, value, timestamp=None):
        for item in self.items:
            if item.value == value:
                self.setPixmap(item.pixmap)
                return
        raise RuntimeError('value "{}" of "{}" not in options ({})'.
                           format(value, box.key(), box.descriptor.options))

    def save(self, e):
        for item in self.items:
            ee = Element(ns_karabo + "option")
            if item.value is not None:
                ee.text = item.value
            if item.url is not None:
                ee.set('image', item.url)
            e.append(ee)

    def load(self, e):
        self.items = []
        for ee in e:
            item = Item(ee, self.project)
            item.value = ee.text
            self.items.append(item)
