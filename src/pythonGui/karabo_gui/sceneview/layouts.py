#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QBoxLayout, QGridLayout, QLayout

from .utils import calc_bounding_rect, save_painter_state


class BaseLayout(object):
    def __init__(self, model, *args):
        super(BaseLayout, self).__init__(*args)
        self.model = model
        self.shapes = []

    def add_shape(self, shape):
        self.shapes.append(shape)

    def add_layout(self, layout):
        """ Needs to be reimplemented in the inherited classes to add a layout.
        """
        raise NotImplementedError("BaseLayout.add_layout")

    def add_widget(self, widget):
        """ Needs to be reimplemented in the inherited classes to add a widget.
        """
        raise NotImplementedError("BaseLayout.add_widget")

    def draw(self, painter):
        for shape in self.shapes:
            with save_painter_state(painter):
                shape.draw(painter)


class GroupLayout(BaseLayout, QLayout):
    def __init__(self, model, parent=None):
        super(GroupLayout, self).__init__(model, parent)
        self._children = []  # contains only QLayoutItems

    def add_layout(self, layout):
        self.addChildLayout(layout)

    def add_widget(self, widget):
        self.addWidget(widget)

    def addItem(self, item):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        self._children.append(item)

    def itemAt(self, index):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        try:
            return self._children[index]
        except IndexError:
            return

    def takeAt(self, index):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
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
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        return len(self._children)

    def setGeometry(self, rect):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        super(GroupLayout, self).setGeometry(rect)

    def sizeHint(self):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        x, y, w, h = calc_bounding_rect(self._children)
        return QSize(w, h)


class BoxLayout(BaseLayout, QBoxLayout):
    def __init__(self, model, direction, parent=None):
        super(BoxLayout, self).__init__(model, direction, parent)
        self.setContentsMargins(5, 5, 5, 5)

    def add_layout(self, layout):
        self.addLayout(layout)

    def add_widget(self, widget):
        self.addWidget(widget)


class GridLayout(BaseLayout, QGridLayout):
    def __init__(self, model, parent=None):
        super(GridLayout, self).__init__(model, parent)

    def add_layout(self, layout):
        self.addLayout(
            layout, layout.model.layout_data.row,
            layout.model.layout_data.colspan, layout.model.layout_data.row,
            layout.model.layout_data.rowspan)

    def add_widget(self, widget):
        self.addWidget(
            widget, widget.model.layout_data.row,
            widget.model.layout_data.colspan, widget.model.layout_data.row,
            widget.model.layout_data.rowspan)
