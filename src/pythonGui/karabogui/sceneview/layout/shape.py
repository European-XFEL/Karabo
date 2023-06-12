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
from qtpy.QtWidgets import QLayout, QLayoutItem

from karabogui.sceneview.utils import save_painter_state


class _ShapeLayoutItemChild(QLayoutItem):
    """ A private QLayoutItem for the ShapeLayoutItem
    """
    def __init__(self, shape):
        super().__init__()
        self.shape = shape

    def isEmpty(self):
        """ This is part of the virtual interface of QLayoutItem.
        """
        return True


class ShapeLayoutItem(QLayout):
    """ A QLayout for holding a single shape object.

    This class exists because Qt apparently doesn't support QLayoutItem
    subclasses. However, since QLayout is a QLayoutItem, you can subclass it to
    get the same behavior when you want to add an item to a layout which isn't
    a layout or a widget (or a spacer). So here we are.
    """
    def __init__(self, shape):
        super().__init__()
        self._item = None
        self.addItem(_ShapeLayoutItemChild(shape))

    # --------------------------------------------
    # SceneView item interface

    @property
    def shape(self):
        return self._item.shape

    @property
    def model(self):
        return self._item.shape.model

    def draw(self, painter):
        shape = self._item.shape
        if shape.is_visible():
            with save_painter_state(painter):
                shape.draw(painter)

    def hide(self):
        self._item.shape.hide()

    def show(self):
        self._item.shape.show()

    def set_geometry(self, rect):
        self._item.shape.set_geometry(rect)

    def translate(self, offset):
        self._item.shape.translate(offset)

    # --------------------------------------------
    # QLayout Virtual Functions

    def addItem(self, item):
        self._item = item

    def count(self):
        return 0 if self._item is None else 1

    def geometry(self):
        return self._item.shape.geometry()

    def itemAt(self, index):
        if index == 0:
            return self._item

    def minimumSize(self):
        return self._item.shape.minimumSize()

    def maximumSize(self):
        return self._item.shape.maximumSize()

    def sizeHint(self):
        return self.minimumSize()

    def setGeometry(self, rect):
        self._item.shape.set_geometry(rect)

    def takeAt(self, index):
        if index == 0:
            self._item = None
        return None
