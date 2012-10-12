#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which a customwidget component
   for the middle panel."""

__all__ = ["GraphicsProxyWidget"]


from layoutcomponents.nodebase import NodeBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsProxyWidget(NodeBase, QGraphicsProxyWidget):


    def __init__(self, widget=None):
        super(GraphicsProxyWidget, self).__init__()

        self._setWidget(widget)
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)


    def __del__(self):
        NodeBase.__del__(self)


    def _setWidget(self, widget):
        self.setWidget(widget)
    def _getWidget(self):
        return self.widget()
    embeddedWidget = property(fget=_getWidget, fset=_setWidget)


### protected ###
    def mouseMoveEvent(self, event):
        QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        QGraphicsItem.mouseReleaseEvent(self, event)


    def contextMenuEvent(self, event):
        pos = event.pos()
        self.embeddedWidget.showContextMenu(QPoint(pos.x(), pos.y()))

