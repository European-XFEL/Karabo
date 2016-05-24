
from karabo_gui.const import ns_karabo
import karabo_gui.icons as icons
from karabo_gui.widget import DisplayWidget
from karabo_gui.messagebox import MessageBox

from karabo.middlelayer import Integer, Number, String

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QByteArray, QBuffer, QDir
from PyQt4.QtGui import (QAction, QApplication, QDialog, QFileDialog, QLabel,
                         QPixmap)

from os import path
import urllib.request
import re
from xml.etree.ElementTree import Element

class IconError(Exception):
    pass

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
        if url is None:
            return
        if not self.getPixmap(project, url):
            return
        self.url = url

    def getPixmap(self, project, url):
        """This function tries to load an image from the given url
        """
        if project is None:
            return False
        
        pixmap = QPixmap()
        try:
            if not pixmap.loadFromData(project.getURL(url)):
                raise IconError
        except (KeyError, IconError):
            MessageBox.showError("Could not read image from URL {}".format(url))
            return False
        
        self.pixmap = pixmap
        return True


class Dialog(QDialog):
    def __init__(self, project, items):
        super(Dialog, self).__init__()
        self.project = project
        self.items = items
        uic.loadUi(path.join(path.dirname(__file__), 'icons.ui'), self)
        self.valueList.addItems([self.textFromItem(item) for item in items])
        self.valueList.setCurrentRow(0)

    @pyqtSlot(int)
    def on_valueList_currentRowChanged(self, row):
        self.image.setPixmap(self.items[row].pixmap)

    def setURL(self, url):
        if url.startswith("file:"):
            url = self.project.addResource("icon", self.project.getURL(url))
        item = self.items[self.valueList.currentRow()]
        if not item.getPixmap(self.project, url):
            return
        item.url = url
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
        name = QFileDialog.getOpenFileName(self, "Open Icon", QDir.homePath(), 
                "Images (*.png *.xpm *.jpg *.jpeg *.svg *.gif *.ico *.tif *.tiff *.bmp)")
        if name:
            url = "file://" + urllib.request.pathname2url(name)
            self.setURL(url)

    @pyqtSlot()
    def on_paste_clicked(self):
        self.on_image_newMime(QApplication.clipboard().mimeData())

    @pyqtSlot()
    def on_copy_clicked(self):
        cr = self.valueList.currentRow()
        url = self.project.addResource("icon",
                                       self.project.getURL(self.items[cr].url))
        self.setURL(url)

    @pyqtSlot()
    def on_deleteValue_clicked(self):
        cr = self.valueList.currentRow()
        if self.items[cr].value is not None:
            self.valueList.takeItem(cr)
            del self.items[cr]

    @pyqtSlot()
    def on_up_clicked(self):
        cr = self.valueList.currentRow()
        if not 0 < cr < len(self.items) - 1:
            return
        self.valueList.insertItem(cr - 1, self.valueList.takeItem(cr))
        self.items[cr - 1], self.items[cr] = self.items[cr], self.items[cr - 1]

    @pyqtSlot()
    def on_down_clicked(self):
        cr = self.valueList.currentRow()
        if cr >= len(self.items) - 2:
            return
        self.valueList.insertItem(cr + 1, self.valueList.takeItem(cr))
        self.items[cr + 1], self.items[cr] = self.items[cr], self.items[cr + 1]

    def exec_(self):
        super(Dialog, self).exec_()
        return self.items

class Icons(DisplayWidget):
    def __init__(self, box, parent):
        super(Icons, self).__init__(box)

        self.widget = QLabel(parent)
        action = QAction("Change Icons...", self.widget)
        self.widget.addAction(action)
        action.triggered.connect(self.showDialog)
        self.items = []

    @classmethod
    def isCompatible(self, box, ro):
        return super().isCompatible(box, ro) and box.descriptor.options is None

    def showDialog(self):
        box = self.boxes[0]
        dialog = self.Dialog(self.project, self.items, box.descriptor)
        self.items = dialog.exec_()
        if box.hasValue():
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
        self.valueList.insertItem(0, self.textValue.text())

    def textFromItem(self, item):
        if item.value is None:
            return "default"
        return item.value


class TextIcons(Icons):
    alias = "Icons"
    category = String
    Dialog = TextDialog

    def valueChanged(self, box, value, timestamp=None):
        for it in self.items:
            if it.value is None or value is not None and it.re.match(value):
                self.setPixmap(it.pixmap)
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
        if isinstance(descriptor, Integer):
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
        self.valueList.insertItem(i, self.textFromItem(item))

    def textFromItem(self, item):
        if item.value is None:
            return "default"
        return "{} {}".format("<=" if item.equal else "<", item.value)


class DigitIcons(Icons):
    alias = "Icons"
    category = Integer, Number
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
        if isinstance(self.boxes[0].descriptor, Integer):
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
            if not any(o == item.value for item in self.items):
                item = Item()
                item.value = o
                items.append(item)

        self.items.extend(items)

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
