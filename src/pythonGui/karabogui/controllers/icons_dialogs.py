import re
import urllib.request
from contextlib import closing
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import QBuffer, QByteArray, pyqtSignal, pyqtSlot
from PyQt4.QtGui import QApplication, QDialog, QLabel, QPixmap
from traits.api import Property

from karabo.common.scenemodel.api import IconData
from karabogui import icons, messagebox
from karabogui.binding.api import FloatBinding, get_min_max
from karabogui.util import getOpenFileName, temp_file


class IconError(Exception):
    pass


def _create_temp_url(item_obj, image_data):
    """Create temporary URL from given ``image_data``."""
    ba = QByteArray()
    buffer = QBuffer(ba)
    buffer.open(QBuffer.WriteOnly)
    with closing(buffer):
        image_data.save(buffer, 'PNG')
        data = buffer.data()
        with temp_file() as tmp_path:
            with open(tmp_path, 'wb') as out:
                out.write(data)
            item_obj.image = tmp_path


class Label(QLabel):
    """A Custom QLabel subclass which is referenced by the 'icons.ui' file
    """
    newMime = pyqtSignal(str)

    def __init__(self, parent):
        super(Label, self).__init__(parent)
        self.setAcceptDrops(True)
        self.setPixmap(None)

    def dragEnterEvent(self, event):
        event.acceptProposedAction()

    def dropEvent(self, event):
        self.newMime.emit(event.mimeData())

    def setPixmap(self, pixmap):
        if pixmap is None:
            QLabel.setPixmap(self, icons.no.pixmap(100))
        else:
            QLabel.setPixmap(self, pixmap)


class IconItem(IconData):
    """Inherit from the scene model `IconData` class to give life to the traits
    which are already declared by that class.
    """
    pixmap = Property()
    re = Property()  # For TextIcons

    def _get_pixmap(self):
        if not self.data:
            pixmap = icons.no.pixmap(100)
            # NOTE: This is nice, the ``_create_temp_url will`` create
            # an image which itself triggers a trait handler to set the data.
            _create_temp_url(self, pixmap)
            return pixmap

        pixmap = QPixmap()
        try:
            if not pixmap.loadFromData(self.data):
                raise IconError
            return pixmap
        except (KeyError, IconError):
            messagebox.show_error("Could not read image.")
        return None

    def _get_re(self):
        return re.compile(self.value)

    def _image_changed(self, url):
        """The `image` trait is the URL of the image data of this item"""
        if not url.startswith("file:"):
            url = "file://" + urllib.request.pathname2url(url)
        self.data = urllib.request.urlopen(url).read()


class _BaseDialog(QDialog):
    def __init__(self, items):
        super(_BaseDialog, self).__init__()
        uic.loadUi(op.join(op.dirname(__file__), 'icons.ui'), self)
        # Get a copy!
        self.items = list(items)
        self.valueList.addItems([self.text_for_item(item) for item in items])
        self.valueList.setCurrentRow(0)

    def _update_image(self, item):
        """Update the ``self.image`` with the given pixmap of the ``item``.
        """
        pixmap = item.pixmap
        if pixmap is None:
            return
        self.image.setPixmap(pixmap)

    @pyqtSlot(int)
    def on_valueList_currentRowChanged(self, row):
        self.image.setPixmap(self.items[row].pixmap)

    def on_image_newMime(self, mime):
        if len(self.items) == 0:
            return

        item = self.items[self.valueList.currentRow()]
        if mime.hasImage():
            _create_temp_url(item, mime.imageData())
        else:
            try:
                filename = mime.text().split()[0].strip()
            except Exception as e:
                e.message = 'Could not open URL or Image'
                raise
            item.image = filename
        self._update_image(item)

    @pyqtSlot()
    def on_open_clicked(self):
        filename = getOpenFileName(parent=self, caption='Open Icon',
                                   filter='Images (*.png *.xpm *.jpg *.jpeg '
                                          '*.svg *.gif *.ico *.tif *.tiff '
                                          '*.bmp)')
        if not filename:
            return

        if len(self.items) == 0:
            return

        item = self.items[self.valueList.currentRow()]
        item.image = filename
        self._update_image(item)

    @pyqtSlot()
    def on_paste_clicked(self):
        self.on_image_newMime(QApplication.clipboard().mimeData())

    @pyqtSlot()
    def on_deleteValue_clicked(self):
        if len(self.items) == 0:
            return

        cr = self.valueList.currentRow()
        if self.items[cr].value is not None:
            self.valueList.takeItem(cr)
            del self.items[cr]

    @pyqtSlot()
    def on_up_clicked(self):
        cr = self.valueList.currentRow()
        if not 0 < cr <= len(self.items) - 1:
            return
        self.valueList.insertItem(cr - 1, self.valueList.takeItem(cr))
        self.items[cr - 1], self.items[cr] = self.items[cr], self.items[cr - 1]
        self.valueList.setCurrentRow(cr - 1)

    @pyqtSlot()
    def on_down_clicked(self):
        cr = self.valueList.currentRow()
        if cr >= len(self.items) - 1:
            return
        self.valueList.insertItem(cr + 1, self.valueList.takeItem(cr))
        self.items[cr + 1], self.items[cr] = self.items[cr], self.items[cr + 1]
        self.valueList.setCurrentRow(cr + 1)


class DigitDialog(_BaseDialog):
    def __init__(self, items, binding):
        super(DigitDialog, self).__init__(items)
        if isinstance(binding, FloatBinding):
            self.value = self.floatValue  # a QSpinBox in icons.ui
            self.stack.setCurrentWidget(self.floatPage)
        else:
            self.value = self.intValue  # a QSpinBox in icons.ui
            self.stack.setCurrentWidget(self.intPage)
        low, high = get_min_max(binding)
        if low is not None:
            self.value.setMinimum(low)
        if high is not None:
            self.value.setMaximum(high)

    @pyqtSlot()
    def on_addValue_clicked(self):
        idx = 0
        number_value = float(self.value.value())
        for idx, item in enumerate(self.items):
            if (number_value < float(item.value) or
                    number_value == float(item.value) and item.equal):
                break

        item = IconItem(value=self.value.value(),
                        equal=self.lessEqual.isChecked())
        self.items.insert(idx, item)
        self.valueList.insertItem(idx, self.text_for_item(item))
        # Trigger the imageView to generate default data!
        self.valueList.setCurrentRow(idx)

    def text_for_item(self, item):
        if item.value is None:
            return 'default'
        return '{} {}'.format('<=' if item.equal else '<', item.value)


class SelectionDialog(_BaseDialog):
    def __init__(self, items, binding):
        super(SelectionDialog, self).__init__(items)
        self.editable.hide()

    def text_for_item(self, item):
        return item.value


class TextDialog(_BaseDialog):
    def __init__(self, items, binding):
        super(TextDialog, self).__init__(items)
        self.stack.setCurrentWidget(self.textPage)

    @pyqtSlot()
    def on_addValue_clicked(self):
        item = IconItem(value=self.textValue.text())
        self.items.insert(0, item)
        self.valueList.insertItem(0, self.textValue.text())
        # Trigger the imageView to generate default data!
        self.valueList.setCurrentRow(0)

    def text_for_item(self, item):
        if not item.value:
            return 'default'
        return item.value
