import hashlib
from os import path
import re
import urllib.request

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QByteArray, QBuffer
from PyQt4.QtGui import QAction, QApplication, QDialog, QLabel, QPixmap

from karabo.middlelayer import Integer, Number, String
import karabo_gui.globals as globals
import karabo_gui.icons as icons
from karabo_gui.messagebox import MessageBox
from karabo_gui.util import getOpenFileName
from karabo_gui.widget import DisplayWidget


class IconError(Exception):
    pass


class Label(QLabel):
    newMime = pyqtSignal(str)

    def __init__(self, parent):
        super(Label, self).__init__(parent)
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


class Item(object):
    value = None  # Property value associated with the image data
    url = None  # Url of the image
    data = None  # Bytes of image data
    pixmap = None  # Pixmap to display the data

    def __init__(self, value, data=None):
        self.value = value
        self.data = data
        self.getPixmap()

    def setURL(self, url):
        self.url = url
        self.data = urllib.request.urlopen(url).read()

    def getPixmap(self):
        """ This function tries to load an image from the given `data` which
            is available as byte string.
        """
        if self.data is None:
            return

        pixmap = QPixmap()
        try:
            if not pixmap.loadFromData(self.data):
                raise IconError
        except (KeyError, IconError):
            MessageBox.showError("Could not read image.")
            return False

        self.pixmap = pixmap
        return True


class Dialog(QDialog):
    def __init__(self, items):
        super(Dialog, self).__init__()
        uic.loadUi(path.join(path.dirname(__file__), 'icons.ui'), self)
        self.items = items
        self.valueList.addItems([self.textFromItem(item) for item in items])
        self.valueList.setCurrentRow(0)

    @pyqtSlot(int)
    def on_valueList_currentRowChanged(self, row):
        self.image.setPixmap(self.items[row].pixmap)

    def setURL(self, url):
        cr = self.valueList.currentRow()
        item = self.items[cr]
        if not url.startswith("file:"):
            url = "file://" + urllib.request.pathname2url(url)
        item.setURL(url)
        if not item.getPixmap():
            return
        self.image.setPixmap(item.pixmap)

    def on_image_newMime(self, mime):
        if mime.hasImage():
            ba = QByteArray()
            try:
                buffer = QBuffer(ba)
                buffer.open(QBuffer.WriteOnly)
                image = mime.imageData().save(buffer, "PNG")
                data = buffer.data()
                image_name = hashlib.sha1(data).hexdigest()
                filename = path.join(globals.HIDDEN_KARABO_FOLDER, image_name)
                # Save to file system for later use
                with open(filename, "wb") as out:
                    out.write(data)
            finally:
                buffer.close()
        else:
            try:
                filename = mime.text().split()[0].strip()
            except Exception as e:
                e.message = "Could not open URL or Image"
                raise
        self.setURL(filename)

    @pyqtSlot()
    def on_open_clicked(self):
        filename = getOpenFileName(parent=self,
                               caption="Open Icon", 
                               filter="Images (*.png *.xpm *.jpg *.jpeg *.svg "
                                      "*.gif *.ico *.tif *.tiff *.bmp)")
        if filename:
            self.setURL(filename)

    @pyqtSlot()
    def on_paste_clicked(self):
        self.on_image_newMime(QApplication.clipboard().mimeData())

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
        dialog = self.Dialog(self.items, box.descriptor)
        self._setItems(dialog.exec_())
        if box.hasValue():
            self.valueChanged(box, box.value)

    def setPixmap(self, p):
        if p is None:
            self.widget.setPixmap(icons.no.pixmap(100))
        else:
            self.widget.setPixmap(p)

    def _setItems(self, items):
        """ Give derived classes a place to respond to changes. """
        self.items = items


class TextDialog(Dialog):
    def __init__(self, items, descriptor):
        super(TextDialog, self).__init__(items)
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


class DigitDialog(Dialog):
    def __init__(self, items, descriptor):
        super(DigitDialog, self).__init__(items)
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


class SelectionDialog(Dialog):
    def __init__(self, items, descriptor):
        super(SelectionDialog, self).__init__(items)
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
        items = list(self.items)
        for o in box.descriptor.options:
            if not any(o == item.value for item in self.items):
                newItem = Item(o)
                items.append(newItem)
        self._setItems(items)

    def valueChanged(self, box, value, timestamp=None):
        for item in self.items:
            if item.value == value:
                self.setPixmap(item.pixmap)
                return
        raise RuntimeError('value "{}" of "{}" not in options ({})'.
                           format(value, box.key(), box.descriptor.options))
