#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QLayout

from .utils import save_painter_state


class GroupLayout(QLayout):
    def __init__(self, parent=None):
        super(GroupLayout, self).__init__(parent)
        self._children = []  # contains only QLayoutItems
        self.shapes = []

    def add_layout(self, layout):
        self.addChildLayout(layout)

    def add_shape(self, shape):
        self.shapes.append(shape)

    def add_widget(self, widget):
        self.addWidget(widget)

    def draw(self, painter):
        for shape in self.shapes:
            with save_painter_state(painter):
                shape.draw(painter)

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
        try:
            ret = self._children.pop(index)
            # XXX: Do we actually need to un-parent items which are removed?
            l = ret.layout()
            if l is not None and l.parent() is self:
                l.setParent(None)
            return ret
        except IndexError:
            return

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
        left, right, top, bottom = 0, 0, 0, 0
        for item in self._children:
            rect = item.geometry()
            left = min(left, rect.left())
            right = max(right, rect.right())
            top = min(top, rect.top())
            bottom = max(bottom, rect.bottom())

        return QSize(bottom - top, right - left)
