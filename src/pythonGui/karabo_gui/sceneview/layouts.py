#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QRect, QSize
from PyQt4.QtGui import QBoxLayout, QGridLayout, QLayout, QWidgetItem

from .utils import calc_bounding_rect, save_painter_state


class BaseLayout(object):
    def __init__(self, model, *args):
        super(BaseLayout, self).__init__(*args)
        self.model = model
        self.shapes = []

    def _add_shape(self, shape):
        self.shapes.append(shape)

    def _remove_shape(self, shape):
        self.shapes.remove(shape)

    def _add_layout(self, layout):
        """ Needs to be reimplemented in the inherited classes to add a layout.
        """
        raise NotImplementedError("BaseLayout.add_layout")

    def _add_widget(self, widget):
        """ Needs to be reimplemented in the inherited classes to add a widget.
        """
        raise NotImplementedError("BaseLayout.add_widget")

    def hide(self):
        for i in range(self.count()):
            item = self.itemAt(i)
            item.hide()

    def show(self):
        for i in range(self.count()):
            item = self.itemAt(i)
            item.show()

    def draw(self, painter):
        for shape in self.shapes:
            if shape.is_visible():
                with save_painter_state(painter):
                    shape.draw(painter)

    def set_geometry(self, rect):
        self.setGeometry(rect)

    def translate(self, offset):
        rect = QRect(self.model.x + offset.x(), self.model.y + offset.y(),
                     self.model.width, self.model.height)
        self.setGeometry(rect)

        for shape in self.shapes:
            shape.translate(offset)
        for i in range(self.count()):
            item = self.itemAt(i)
            if isinstance(item, BaseLayout):
                item.translate(offset)
            elif isinstance(item, QWidgetItem):
                item.widget().translate(offset)

    def geometry(self):
        """ This is part of the virtual interface of QLayout.
        """
        return QRect(self.model.x, self.model.y, self.model.width,
                     self.model.height)

    def setGeometry(self, rect):
        """ This is part of the virtual interface of QLayout.
        """
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        super(BaseLayout, self).setGeometry(rect)


class GroupLayout(BaseLayout, QLayout):
    def __init__(self, model, parent=None):
        super(GroupLayout, self).__init__(model, parent)
        self._children = []  # contains only QLayoutItems

    def _add_layout(self, layout):
        self.addChildLayout(layout)

    def _remove_layout(self, layout):
        self.removeItem(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)

    def _remove_widget(self, widget):
        self.removeWidget(widget)

    def addItem(self, item):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        self._children.append(item)

    def removeItem(self, item):
        """ DO NOT CALL THIS METHOD DIRECTLY!

        This is part of the virtual interface of QLayout.
        """
        self._children.remove(item)
        super(GroupLayout, self).removeItem(item)

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

    def _add_layout(self, layout):
        self.addLayout(layout)

    def _remove_layout(self, layout):
        self.removeItem(layout)

    def _add_widget(self, widget):
        self.addWidget(widget)

    def _remove_widget(self, widget):
        self.removeWidget(widget)


class GridLayout(BaseLayout, QGridLayout):
    def __init__(self, model, parent=None):
        super(GridLayout, self).__init__(model, parent)

    def _add_layout(self, layout):
        self.addLayout(
            layout, layout.model.layout_data.row,
            layout.model.layout_data.colspan, layout.model.layout_data.row,
            layout.model.layout_data.rowspan)

    def _remove_layout(self, layout):
        self.removeItem(layout)

    def _add_widget(self, widget):
        self.addWidget(
            widget, widget.model.layout_data.row,
            widget.model.layout_data.colspan, widget.model.layout_data.row,
            widget.model.layout_data.rowspan)

    def _remove_widget(self, widget):
        self.removeWidget(widget)
