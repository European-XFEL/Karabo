#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
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
#############################################################################
from qtpy.QtCore import QRect, QSize
from qtpy.QtWidgets import QBoxLayout, QGridLayout, QLayout, QWidgetItem

from karabogui.sceneview.utils import calc_bounding_rect

from .base import BaseLayout


class GroupLayout(BaseLayout, QLayout):
    def __init__(self, model, parent=None):
        super().__init__(model, parent)
        self._children = []
        self._child_bounds = None

    def _add_layout(self, layout):
        self.addItem(layout)
        super()._add_layout(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)

    # --------------------------------------------
    # QLayout Virtual Functions

    def addItem(self, item):
        self._children.append(item)
        self._child_bounds = None

    def removeItem(self, item):
        self._children.remove(item)
        self._child_bounds = None
        super().removeItem(item)

    def itemAt(self, index):
        try:
            return self._children[index]
        except IndexError:
            return

    def takeAt(self, index):
        item = self._children.pop(index)
        self._child_bounds = None
        layout = item.layout()
        if layout is not None and layout.parent() is self:
            layout.setParent(None)
        # The creation path of the layout items bypasses the virtual
        # wrapper methods, this means that the ownership of the cpp
        # pointer is never transfered to Qt. If the item is returned
        # here it will be delete by Qt, which doesn't own the pointer.
        # A double free occurs once the Python item falls out of scope.
        # To avoid this, this method always returns None and the item
        # cleanup is performed by Python, which owns the cpp pointer.
        del item
        return None

    def count(self):
        return len(self._children)

    def geometry(self):
        model = self.model
        rect = QRect(model.x, model.y, model.width, model.height)
        if rect.isEmpty():
            rect = QRect(*self._child_bounding_rect())
        return rect

    def maximumSize(self):
        return self.sizeHint()

    def minimumSize(self):
        return self.sizeHint()

    def sizeHint(self):
        x, y, w, h = self._child_bounding_rect()
        return QSize(w, h)

    def setGeometry(self, rect):
        curr_geometry = self.geometry()
        if rect == curr_geometry:
            # NOTE: If we don't bail out here, Qt will continue to call us!
            # I have no idea why that is, but it will eat your CPU!
            return

        # Translate all children by the amount our origin moved
        offset = rect.topLeft() - curr_geometry.topLeft()
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().translate(offset)
            else:
                item.translate(offset)
        # Invalidate the cached bounds!
        self._child_bounds = None

        super().setGeometry(rect)

    def _child_bounding_rect(self):
        """Cache the result of calc_bounding_rect to avoid expensive
        recalculation.
        """
        if self._child_bounds is None:
            self._child_bounds = calc_bounding_rect(self._children)
        return self._child_bounds


VERTICAL_LAYOUTS = (QBoxLayout.TopToBottom, QBoxLayout.BottomToTop)


class BoxLayout(BaseLayout, QBoxLayout):
    def __init__(self, model, direction, parent=None):
        super().__init__(model, direction, parent)
        self.setContentsMargins(0, 0, 0, 0)

    def _add_layout(self, layout):
        self.addItem(layout)
        super()._add_layout(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)

    def minimumSize(self):
        # We don't set a minimum size on the layout to enable more flexible
        # resizing
        return QSize()

    def sizeHint(self):
        """Calculate the sizeHint of the layout from the sizeHints of the
           children. This varies with direction."""

        # We compute the sizeHint only if the model is invalid.
        # We use the child bounding rects if otherwise.
        model = self.model
        if QRect(model.x, model.y, model.width, model.height).isEmpty():
            sizes = [self.itemAt(i).sizeHint() for i in range(self.count())]
            widths = [size.width() for size in sizes]
            heights = [size.height() for size in sizes]

            get_width, get_height = sum, max
            if self.direction() in VERTICAL_LAYOUTS:
                get_width, get_height = get_height, get_width

            return QSize(get_width(widths), get_height(heights))

        return super().sizeHint()

    def setGeometry(self, rect):
        super().setGeometry(rect)

        # Update the geometry of the widget in their model
        for i in range(self.count()):
            item = self.itemAt(i)
            rect = item.geometry()

            if isinstance(item, BaseLayout):
                model = item.model
            elif isinstance(item, QWidgetItem):  # QWidgetItem
                model = item.widget().model
            else:
                # This is probably a ShapeLayoutItem. It has a `set_geometry`
                # that sets the values to its model. We do nothing instead.
                return

            # Update the model geometry
            model.trait_set(x=rect.x(), y=rect.y(),
                            width=rect.width(), height=rect.height())


class GridLayout(BaseLayout, QGridLayout):
    def __init__(self, model, parent=None):
        super().__init__(model, parent)

    def _add_layout(self, layout):
        ld = layout.model.layout_data
        self.addLayout(layout, ld.row, ld.col, ld.rowspan, ld.colspan)
        super()._add_layout(layout)

    def _add_widget(self, widget):
        ld = widget.model.layout_data
        self.addWidget(widget, ld.row, ld.col, ld.rowspan, ld.colspan)
