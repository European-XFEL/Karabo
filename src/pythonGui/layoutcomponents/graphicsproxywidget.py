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
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseMoveEvent(self, event)
        else:
            QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        if self.isEditable == True:
            QGraphicsProxyWidget.mousePressEvent(self, event)
        else:
            QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseReleaseEvent(self, event)
        else:
            QGraphicsItem.mouseReleaseEvent(self, event)


    #def keyPressEvent(self, event):
    #    print "keyPressEvent"
    #    if self.isEditable == True:
    #        QGraphicsProxyWidget.keyPressEvent(self, event)
    #    else:
    #        QGraphicsItem.keyPressEvent(self, event)


    #def keyReleaseEvent(self, event):
    #    print "keyReleaseEvent"
    #    if self.isEditable == True:
    #        QGraphicsProxyWidget.keyReleaseEvent(self, event)
    #    else:
    #        QGraphicsItem.keyReleaseEvent(self, event)


    def contextMenuEvent(self, event):
        pos = event.pos()
        self.embeddedWidget.showContextMenu(QPoint(pos.x(), pos.y()))

