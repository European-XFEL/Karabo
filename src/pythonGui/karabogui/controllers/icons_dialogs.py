from contextlib import closing
import os.path as op
import re
import sys
import urllib.request
from urllib.error import URLError

from PyQt5 import uic
from PyQt5.QtCore import pyqtSignal, pyqtSlot, QBuffer, QByteArray, Qt
from PyQt5.QtGui import QPixmap, QPixmapCache
from PyQt5.QtWidgets import QApplication, QDialog, QLabel
from traits.api import Instance, Property

from karabo.common.scenemodel.api import IconData
from karabogui import icons, messagebox
from karabogui.binding.api import FloatBinding, get_min_max
from karabogui.util import getOpenFileName, temp_file


ICON_FILE_SIZE_LIMIT = 102400  # 100 KB


class IconError(Exception):
    pass


class IconFileSizeLimitError(Exception):
    """Exception for icons exceeding file size limit"""


class IconLabel(QLabel):

    def __init__(self, parent=None):
        super(IconLabel, self).__init__(parent=parent)
        self.setMinimumSize(24, 24)
        self._pixmap = self._default_pixmap = icons.no.pixmap(100)

    # Qt Overrides
    # ---------------------------------------------------------------------

    def resizeEvent(self, event):
        """Reimplemented method to reset the pixmap scale with the
           widget resize"""
        if self._pixmap is not None:
            super(IconLabel, self).setPixmap(self._scaled_pixmap)

        super(IconLabel, self).resizeEvent(event)

    def setPixmap(self, pixmap):
        """Reimplemented method to store the input pixmap and use the
           scaled version internally"""
        self._pixmap = pixmap
        super(IconLabel, self).setPixmap(self._scaled_pixmap)

    # Properties
    # ---------------------------------------------------------------------

    @property
    def _scaled_pixmap(self):
        """Scales the pixmap only if the pixmap is larger than the widget"""
        pixmap = self._pixmap
        if self._is_pixmap_big:
            pixmap = pixmap.scaled(self.size(),
                                   Qt.KeepAspectRatio, Qt.SmoothTransformation)

        return pixmap

    @property
    def _is_pixmap_big(self):
        """Checks if the pixmap is larger than the widget by comparing
           the widths and heights"""
        pixmap_size = self._pixmap.size()
        widget_size = self.size()
        horizontally_big = pixmap_size.width() > widget_size.width()
        vertically_big = pixmap_size.height() > widget_size.height()
        return horizontally_big or vertically_big

    def setDefaultPixmap(self):
        self.setPixmap(self._default_pixmap)


class Label(IconLabel):
    """A Custom QLabel subclass which is referenced by the 'icons.ui' file
    """
    newMime = pyqtSignal(str)

    def __init__(self, parent):
        super(Label, self).__init__(parent)
        self.setAcceptDrops(True)

    def dragEnterEvent(self, event):
        event.acceptProposedAction()

    def dropEvent(self, event):
        self.newMime.emit(event.mimeData())


class IconItem(IconData):
    """Inherit from the scene model `IconData` class to give life to the traits
    which are already declared by that class.
    """
    pixmap = Property(Instance(QPixmap))
    re = Property()  # For TextIcons

    def _get_pixmap(self):
        # If no data is added, we use the default pixmap
        if not self.data:
            # If the default pixmap url is not yet existing, we add the pixmap
            pixmap = icons.no.pixmap(100)
            self._cache_pixmap(pixmap, update=True)
            return pixmap

        # If image does not have a temp url, the item is initially loaded from
        # the model. It needs to be saved in a temp file and has to be added
        # in the pixmap cache
        if not self.image:
            pixmap = self._load_pixmap()
            self._cache_pixmap(pixmap)
            return pixmap

        # Get pixmap from cache
        pixmap = QPixmapCache.find(self.image)

        # If the pixmap is not in the cache, we need to load and add the data.
        if pixmap is None:
            pixmap = self._load_pixmap()
            self._cache_pixmap(pixmap)

        return pixmap

    def _get_re(self):
        return re.compile(self.value)

    def _image_changed(self, old_url, new_url):
        """The `image` trait is the URL of the image data of this item"""
        if not new_url.startswith("file:"):
            new_url = "file://" + urllib.request.pathname2url(new_url)
        try:
            data = urllib.request.urlopen(new_url).read()

            # Check if data size is larger than the limit
            file_size = sys.getsizeof(data)
            if file_size > ICON_FILE_SIZE_LIMIT:
                raise IconFileSizeLimitError

            # Try to load the file as QPixmap to check image validity
            pixmap = QPixmap()
            if not pixmap.loadFromData(data):
                raise IconError
        except URLError:
            # Revert changes
            self.trait_setq(image=old_url)
            messagebox.show_error("Pasted image is invalid.")
        except IconFileSizeLimitError:
            self.trait_setq(image=old_url)
            message = ("Desired file exceeds the file size limit of "
                       "{size_limit} KB [{file_size:.2f} KB]."
                       .format(size_limit=int(ICON_FILE_SIZE_LIMIT / 1024),
                               file_size=file_size / 1024))
            messagebox.show_error(message)
        except IconError:
            self.trait_setq(image=old_url)
            messagebox.show_error("Could not read image.")
        else:
            self.data = data

    def _load_pixmap(self):
        """Load the pixmap from the saved bytes"""
        pixmap = QPixmap()
        try:
            if not pixmap.loadFromData(self.data):
                raise IconError
        except (KeyError, IconError):
            messagebox.show_error("Could not read image.")
            pixmap = icons.no.pixmap(100)

        return pixmap

    def _cache_pixmap(self, pixmap, update=False):
        """Save the pixmap in the cache, with a temporary file as key.
           Optionally update the data, which is usually not desired as it is
           already previously loaded."""
        temp_url = self.create_temp_url(pixmap, update=update)
        QPixmapCache.insert(temp_url, pixmap)

    def create_temp_url(self, image_data, update=True):
        """Create temporary URL from given ``image_data``."""
        ba = QByteArray()
        buff = QBuffer(ba)
        buff.open(QBuffer.WriteOnly)
        with closing(buff):
            image_data.save(buff, 'PNG')
            data = buff.data()
            with temp_file() as tmp_path:
                with open(tmp_path, 'wb') as out:
                    out.write(data)

        if update:
            self.data = data.data()
        self.trait_setq(image=tmp_path)

        return tmp_path


class _BaseDialog(QDialog):
    def __init__(self, items, parent=None):
        super(_BaseDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), 'icons.ui'), self)
        # Get a copy!
        self.items = list(items)
        self.valueList.addItems([self.text_for_item(item) for item in items])
        self.valueList.setCurrentRow(0)
        self._update_buttons()

    def _update_buttons(self):
        """Only allow Open and Paste buttons if the rows contains something
        """
        enabled = len(self.items) > 0
        self.open.setEnabled(enabled)
        self.paste.setEnabled(enabled)
        self.deleteValue.setEnabled(enabled)

    def _update_image(self, item):
        """Update the ``self.image`` with the given pixmap of the ``item``.
        """
        pixmap = item.pixmap
        if pixmap is None:
            return
        self.image.setPixmap(pixmap)

    @pyqtSlot(int)
    def on_valueList_currentRowChanged(self, row):
        self._update_buttons()
        self.image.setPixmap(self.items[row].pixmap)

    def on_image_newMime(self, mime):
        if len(self.items) == 0:
            messagebox.show_error("No item is selected, aborting...")
            return

        item = self.items[self.valueList.currentRow()]
        if mime.hasImage():
            item.create_temp_url(mime.imageData())
        else:
            try:
                filename = mime.text().strip()
            except Exception as e:
                e.message = 'Could not open URL or Image'
                raise
            item.image = filename
        self._update_image(item)

    @pyqtSlot()
    def on_open_clicked(self):
        filename = getOpenFileName(
            parent=self, caption='Open Icon',
            filter='Images (*.png *.xpm *.jpg *.jpeg *.svg *.gif *.ico '
                   '*.tif *.tiff *.bmp)')
        if not filename:
            return

        if len(self.items) == 0:
            messagebox.show_error("No item is selected, aborting...")
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
            if len(self.items) == 0:
                self.image.setDefaultPixmap()
        self._update_buttons()

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
    def __init__(self, items, binding, parent=None):
        super(DigitDialog, self).__init__(items, parent)
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
        entry_value = self.value.value()
        entry_equal = self.lessEqual.isChecked()
        for idx, item in enumerate(self.items):
            # Check if entry is already existing
            item_value = float(item.value)
            if entry_value == item_value and entry_equal is item.equal:
                message = "Cannot add new condition; it already exists."
                messagebox.show_error(message, parent=self)
                return

            # Find new position in list widget
            if (entry_value < item_value or
                    entry_value == item_value and item.equal):
                break

        item = IconItem(value=entry_value, equal=entry_equal)
        self.items.insert(idx, item)
        self.valueList.insertItem(idx, self.text_for_item(item))
        # Trigger the imageView to generate default data!
        self.valueList.setCurrentRow(idx)

    def text_for_item(self, item):
        if item.value is None:
            return 'default'
        return '{} {}'.format('<=' if item.equal else '<', item.value)


class SelectionDialog(_BaseDialog):
    def __init__(self, items, binding, parent=None):
        super(SelectionDialog, self).__init__(items, parent)
        self.editable.hide()

    def text_for_item(self, item):
        return item.value


class TextDialog(_BaseDialog):
    def __init__(self, items, binding, parent=None):
        super(TextDialog, self).__init__(items, parent)
        self.stack.setCurrentWidget(self.textPage)

    @pyqtSlot()
    def on_addValue_clicked(self):
        entry_value = self.textValue.text()
        # Check if entry is already existing
        if entry_value in [item.value for item in self.items]:
            message = "Cannot add new condition; it already exists."
            messagebox.show_error(message, parent=self)
            return

        item = IconItem(value=entry_value)
        self.items.insert(0, item)
        self.valueList.insertItem(0, entry_value)
        # Trigger the imageView to generate default data!
        self.valueList.setCurrentRow(0)

    def text_for_item(self, item):
        if not item.value:
            return 'default'
        return item.value
