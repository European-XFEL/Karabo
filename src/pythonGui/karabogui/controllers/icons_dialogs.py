# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import re
import sys
import urllib.request
from contextlib import closing
from pathlib import Path
from urllib.error import URLError

from qtpy import uic
from qtpy.QtCore import QBuffer, QByteArray, Qt, Signal, Slot
from qtpy.QtGui import QPalette, QPixmap, QPixmapCache
from qtpy.QtWidgets import (
    QApplication, QDialog, QLabel, QLineEdit, QSizePolicy)
from traits.api import Instance, Property

from karabo.common.scenemodel.api import IconData
from karabogui import icons, messagebox
from karabogui.binding.api import FloatBinding, get_min_max
from karabogui.singletons.api import get_config
from karabogui.util import getOpenFileName, temp_file
from karabogui.validators import IntValidator, NumberValidator

ICON_FILE_SIZE_LIMIT = 102400  # 100 KB
MIN_SIZE = 24
DEFAULT_SIZE = 100


class IconError(Exception):
    pass


class IconFileSizeLimitError(Exception):
    """Exception for icons exceeding file size limit"""


class IconLabel(QLabel):

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setMinimumSize(MIN_SIZE, MIN_SIZE)
        self.setSizePolicy(QSizePolicy.MinimumExpanding,
                           QSizePolicy.MinimumExpanding)
        self._pixmap = self._default_pixmap = icons.no.pixmap(100)

    # Qt Overrides
    # ---------------------------------------------------------------------

    def resizeEvent(self, event):
        """Reimplemented method to scale the QPixmap"""
        super().resizeEvent(event)
        self.setPixmap(self._pixmap)

    def setPixmap(self, pixmap):
        """Reimplemented method to store the input pixmap and use the
           scaled version"""
        self._pixmap = pixmap
        size = self.size()
        pixmap = pixmap.scaled(size, Qt.KeepAspectRatio,
                               Qt.SmoothTransformation)
        super().setPixmap(pixmap)

    def setDefaultPixmap(self):
        self.setPixmap(self._default_pixmap)


class Label(QLabel):
    """A Custom QLabel subclass which is referenced by the 'icons.ui' file

    This Label shrinks a Pixmap if necessary to the size, but by default
    does not enlarge a label if there is more space.
    """
    newMime = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setAcceptDrops(True)
        self._pixmap = self._default_pixmap = icons.no.pixmap(100)

    # Qt Overrides
    # ---------------------------------------------------------------------

    def resizeEvent(self, event):
        """Reimplemented method to reset the pixmap scale with the
           widget resize"""
        if self._pixmap is not None:
            super().setPixmap(self._scaled_pixmap)

        super().resizeEvent(event)

    def setPixmap(self, pixmap):
        """Reimplemented method to store the input pixmap and use the
           scaled version internally"""
        self._pixmap = pixmap
        super().setPixmap(self._scaled_pixmap)

    def dragEnterEvent(self, event):
        event.acceptProposedAction()

    def dropEvent(self, event):
        self.newMime.emit(event.mimeData())

    # Properties
    # ---------------------------------------------------------------------

    @property
    def _scaled_pixmap(self):
        """Scales the pixmap only if the pixmap is larger than the widget"""
        pixmap = self._pixmap
        if self.needs_shrink:
            pixmap = pixmap.scaled(self.size(),
                                   Qt.KeepAspectRatio, Qt.SmoothTransformation)

        return pixmap

    @property
    def needs_shrink(self):
        """Checks if the pixmap is larger than the widget by comparing
           the widths and heights"""
        pixmap_size = self._pixmap.size()
        widget_size = self.size()
        horizontally_big = pixmap_size.width() > widget_size.width()
        vertically_big = pixmap_size.height() > widget_size.height()
        return horizontally_big or vertically_big

    def setDefaultPixmap(self):
        self.setPixmap(self._default_pixmap)


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
            pixmap = icons.no.pixmap(DEFAULT_SIZE, DEFAULT_SIZE)
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
            pixmap = icons.no.pixmap(DEFAULT_SIZE, DEFAULT_SIZE)

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
            image_data.save(buff, "PNG")
            data = buff.data()
            with temp_file() as tmp_path:
                with open(tmp_path, "wb") as out:
                    out.write(data)

        if update:
            self.data = data.data()
        self.trait_setq(image=tmp_path)

        return tmp_path


class _BaseIconDialog(QDialog):
    def __init__(self, items, parent=None):
        super().__init__(parent)
        uic.loadUi(Path(__file__).parent.joinpath("icons.ui"), self)
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

    @Slot(int)
    def on_valueList_currentRowChanged(self, row):
        """Update the buttons on every selection change and select the pixmap
        """
        self._update_buttons()
        self.image.setPixmap(self.items[row].pixmap)

    @Slot()
    def on_open_clicked(self):
        """Slot executing on `open` button"""
        filename = getOpenFileName(
            parent=self, caption="Open Icon",
            filter="Images (*.png *.xpm *.jpg *.jpeg *.svg *.gif *.ico "
                   "*.tif *.tiff *.bmp)", directory=get_config()["data_dir"])
        if not filename:
            return

        get_config()["data_dir"] = filename
        item = self.items[self.valueList.currentRow()]
        item.image = filename
        self._update_image(item)

    @Slot()
    def on_paste_clicked(self):
        """Slot executing on `paste` button"""
        mime = QApplication.clipboard().mimeData()
        item = self.items[self.valueList.currentRow()]
        if mime.hasImage():
            item.create_temp_url(mime.imageData())
        else:
            try:
                filename = mime.text().strip()
            except Exception as e:
                e.message = "Could not open URL or Image"
                raise
            item.image = filename
        self._update_image(item)

    @Slot()
    def on_deleteValue_clicked(self):
        """Slot executing on `delete` button"""
        row = self.valueList.currentRow()
        if self.items[row].value is not None:
            self.valueList.takeItem(row)
            del self.items[row]
            if len(self.items) == 0:
                self.image.setDefaultPixmap()
        self._update_buttons()

    # --------------------------------------------------------------------
    # Abstract interface

    def text_for_item(self, item):
        pass


class LineEditEditor(QLineEdit):

    def __init__(self, parent=None):
        super().__init__(parent)
        self._normal_palette = self.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        self.textChanged.connect(self._check_background)

    @Slot(str)
    def _check_background(self, text):
        acceptable_input = self.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.setPalette(palette)


class DigitDialog(_BaseIconDialog):
    def __init__(self, items, binding, parent=None):
        super().__init__(items, parent)
        self.value = LineEditEditor(parent=self)
        minInc, maxInc = get_min_max(binding)
        if isinstance(binding, FloatBinding):
            validator = NumberValidator(minInc, maxInc)
            self.float_layout.insertWidget(0, self.value)
            self.stack.setCurrentWidget(self.floatPage)
            self.cast = float
        else:
            validator = IntValidator(minInc, maxInc)
            self.int_layout.insertWidget(0, self.value)
            self.stack.setCurrentWidget(self.intPage)
            self.cast = int
        # Set the validator on the line edit
        self.value.setValidator(validator)

    @Slot()
    def on_addValue_clicked(self):
        if not self.value.hasAcceptableInput():
            text = (f"Value {self.value.text()} is not a valid value or in "
                    "the range of the property limits.")
            messagebox.show_error(text, parent=self)
            return

        new_idx = 0
        entry_value = self.cast(self.value.text())
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
                new_idx = idx
                break

            new_idx = idx + 1

        item = IconItem(value=entry_value, equal=entry_equal)
        self.items.insert(new_idx, item)
        self.valueList.insertItem(new_idx, self.text_for_item(item))
        # Trigger the imageView to generate default data!
        self.valueList.setCurrentRow(new_idx)

    def text_for_item(self, item):
        if item.value is None:
            return "default"
        return "{} {}".format("<=" if item.equal else "<", item.value)


class SelectionDialog(_BaseIconDialog):
    def __init__(self, items, binding, parent=None):
        super().__init__(items, parent)
        self.editable.hide()

    def text_for_item(self, item):
        return item.value


class TextDialog(_BaseIconDialog):
    def __init__(self, items, binding, parent=None):
        super().__init__(items, parent)
        self.stack.setCurrentWidget(self.textPage)

        if binding.options:
            self.stack.setCurrentWidget(self.textOptionsPage)
            self.itemsComboBox.addItems(list(binding.options))

    @Slot()
    def on_addValue_clicked(self):
        if self.stack.currentWidget() == self.textPage:
            entry_value = self.textValue.text().strip()
        else:
            entry_value = self.itemsComboBox.currentText()
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
            return "default"
        return item.value
