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
from qtpy.QtCore import QRect
from qtpy.QtWidgets import QWidgetItem

from .shape import ShapeLayoutItem


class BaseLayout:
    """ This is a mix-in class intended to be inherited by all SceneView
    layouts. It implements generic parts of the QLayout virtual interface.
    """
    def __init__(self, model, *args):
        super().__init__(*args)
        self.model = model
        self._drawable_items = []

    def __iter__(self):
        """ Implement the Python iterator protocol for all layouts.
        """
        for i in range(self.count()):
            yield self.itemAt(i)

    def _add_layout(self, layout):
        """This MUST be called by inherited classes if they overload it!
        """
        self._drawable_items.append(layout)

    def _add_shape(self, shape):
        """ Use our special hacky workaround for adding shapes to the layout.
        """
        item = ShapeLayoutItem(shape)
        self._add_layout(item)
        self._drawable_items.append(item)

    def _add_widget(self, widget):
        """ Needs to be reimplemented in the inherited classes to add a widget.
        """
        raise NotImplementedError("BaseLayout._add_widget")

    def _remove_layout(self, layout):
        self.removeItem(layout)
        self._drawable_items.remove(layout)

    def _remove_shape(self, shape):
        for item in self:
            if isinstance(item, ShapeLayoutItem) and item.model is shape.model:
                # Call QLayout::removeItem()
                self.removeItem(item)
                self._drawable_items.remove(item)
                return

    def _remove_widget(self, widget):
        self.removeWidget(widget)

    def draw(self, painter):
        """Draw all the things which have a draw() method"""
        for item in self._drawable_items:
            item.draw(painter)

    def hide(self):
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().hide()
            else:
                item.hide()

    def show(self):
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().show()
            else:
                item.show()

    def set_geometry(self, rect):
        self.setGeometry(rect)

    def translate(self, offset):
        rect = QRect(self.model.x + offset.x(), self.model.y + offset.y(),
                     self.model.width, self.model.height)
        self.set_geometry(rect)

    # --------------------------------------------
    # QLayout Virtual Functions

    def geometry(self):
        return QRect(self.model.x, self.model.y, self.model.width,
                     self.model.height)

    def setGeometry(self, rect):
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.invalidate()  # Important! Force a full layout recalculation
        super().setGeometry(rect)

    def sizeHint(self):
        return self.geometry().size()
