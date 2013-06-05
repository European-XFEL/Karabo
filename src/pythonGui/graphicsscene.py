#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsScene."""

__all__ = ["GraphicsScene"]

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsScene(QGraphicsScene):


    def __init__(self, x, y, width, height):
        super(GraphicsScene, self).__init__(x, y, width, height)


    def breakLayout(self, layoutContainer):
        extractedItems = []
        layout = layoutContainer.layout()
        nbItems = layout.count()
        while nbItems > 0:
            nbItems -= 1
            
            item = layout.itemAt(nbItems)
            pos = item.pos()

            item.setFlag(QGraphicsItem.ItemIsMovable, True)
            item.setFlag(QGraphicsItem.ItemIsSelectable, True)
            extractedItems.append(item)
            
            # Remove item from layout
            layout.removeItem(item)
            # Remove item
            self.removeItem(item)
            # Add item to scene
            self.addItem(item)
            scenePos = layoutContainer.mapToScene(pos)
            item.adjustSize()
            item.setPos(scenePos)

