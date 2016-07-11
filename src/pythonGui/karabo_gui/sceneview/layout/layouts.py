#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QRect, QSize
from PyQt4.QtGui import QBoxLayout, QGridLayout, QLayout, QWidgetItem

from karabo_gui.sceneview.utils import calc_bounding_rect
from .base import BaseLayout


class GroupLayout(BaseLayout, QLayout):
    def __init__(self, model, parent=None):
        super(GroupLayout, self).__init__(model, parent)
        self._children = []

    def _add_layout(self, layout):
        self.addItem(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)

    # --------------------------------------------
    # QLayout Virtual Functions

    def addItem(self, item):
        self._children.append(item)

    def removeItem(self, item):
        self._children.remove(item)
        super(GroupLayout, self).removeItem(item)

    def itemAt(self, index):
        try:
            return self._children[index]
        except IndexError:
            return

    def takeAt(self, index):
        item = self._children.pop(index)
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
        return None

    def count(self):
        return len(self._children)

    def geometry(self):
        model = self.model
        rect = QRect(model.x, model.y, model.width, model.height)
        if rect.isEmpty():
            rect = QRect(*calc_bounding_rect(self._children))
        return rect

    def maximumSize(self):
        return self.sizeHint()

    def minimumSize(self):
        return self.sizeHint()

    def sizeHint(self):
        x, y, w, h = calc_bounding_rect(self._children)
        return QSize(w, h)

    def setGeometry(self, rect):
        # Translate all children by the amount our origin moved
        offset = rect.topLeft() - self.geometry().topLeft()
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().translate(offset)
            else:
                item.translate(offset)

        super(GroupLayout, self).setGeometry(rect)


class BoxLayout(BaseLayout, QBoxLayout):
    def __init__(self, model, direction, parent=None):
        super(BoxLayout, self).__init__(model, direction, parent)
        self.setContentsMargins(5, 5, 5, 5)

    def _add_layout(self, layout):
        self.addItem(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)


class GridLayout(BaseLayout, QGridLayout):
    def __init__(self, model, parent=None):
        super(GridLayout, self).__init__(model, parent)

    def _add_layout(self, layout):
        ld = layout.model.layout_data
        self.addLayout(layout, ld.row, ld.col, ld.rowspan, ld.colspan)

    def _add_widget(self, widget):
        ld = widget.model.layout_data
        self.addWidget(widget, ld.row, ld.col, ld.rowspan, ld.colspan)
