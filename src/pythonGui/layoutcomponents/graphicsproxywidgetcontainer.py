#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsWidget containing a
   layout with customized widgets.
"""

__all__ = ["GraphicsProxyWidgetContainer"]


from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.nodebase import NodeBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsProxyWidgetContainer(NodeBase, QGraphicsWidget):

    def __init__(self, isEditableMode):
        super(GraphicsProxyWidgetContainer, self).__init__(isEditableMode)

        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
        
        self._setupContextMenu()


    def destroy(self):
        layout = self.layout()
        nbItems = layout.count()
        while nbItems > 0:
            nbItems -= 1
            item = layout.itemAt(nbItems)
            if isinstance(item, GraphicsProxyWidget):
                item.destroy()


    def _setupContextMenu(self):
        # Populate context menu
        self.__contextMenu = QMenu()
        
        text = "Horizontal layout"
        self.__acHLayout = QAction(text, self)
        self.__acHLayout.setStatusTip(text)
        self.__acHLayout.setToolTip(text)
        self.__acHLayout.triggered.connect(self.onHorizontalLayout)
        self.__contextMenu.addAction(self.__acHLayout)
        
        text = "Vertical layout"
        self.__acVLayout = QAction(text, self)
        self.__acVLayout.setStatusTip(text)
        self.__acVLayout.setToolTip(text)
        self.__acVLayout.triggered.connect(self.onVerticalLayout)
        self.__contextMenu.addAction(self.__acVLayout)
        
        self.__contextMenu.addSeparator()

        text = "Break layout"
        self.__acBreakLayout = QAction(text, self)
        self.__acBreakLayout.setStatusTip(text)
        self.__acBreakLayout.setToolTip(text)
        self.__acBreakLayout.triggered.connect(self.onBreakLayout)
        self.__contextMenu.addAction(self.__acBreakLayout)


    def _showContextMenu(self, pos):
        self.scene().clearSelection()
        self.setSelected(True)

        self.__contextMenu.exec_(pos)


### protected ###
    def paint(self, painter, option, widget):
        if self.isDesignMode and self.isSelected():
            pen = painter.pen()
            pen.setStyle(Qt.DashLine)
            painter.setPen(pen)
            #painter.setBrush(QColor(255,255,200))
            rect = self.boundingRect()
            painter.drawRect(rect)
        QGraphicsWidget.paint(self, painter, option, widget)
   

    def mouseMoveEvent(self, event):
        #print "+++ QGraphicsWidget.mouseMoveEvent", self.isDesignMode
        if self.isDesignMode == False:
            QGraphicsWidget.mouseMoveEvent(self, event)
            event.accept()
        else:
            QGraphicsItem.mouseMoveEvent(self, event)

   
    def mousePressEvent(self, event):
        #print "+++ QGraphicsWidget.mousePressEvent", self.isDesignMode
        if self.isDesignMode == False:
            self.setFlag(QGraphicsItem.ItemIsMovable, False)
            QGraphicsWidget.mousePressEvent(self, event)
            event.accept()
        else:
            self.setFlag(QGraphicsItem.ItemIsMovable, True)
            QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        #print "+++ QGraphicsWidget.mouseReleaseEvent", self.isDesignMode
        if self.isDesignMode == False:
            QGraphicsWidget.mouseReleaseEvent(self, event)
            event.accept()
        else:
            QGraphicsItem.mouseReleaseEvent(self, event)
    

    def contextMenuEvent(self, event):
        #print "+++ QGraphicsWidget.contextMenuEvent"
        if self.isDesignMode:
            self._showContextMenu(event.screenPos())
        QGraphicsWidget.contextMenuEvent(self, event)
            
            
    def focusInEvent(self, event):
        if not self.isDesignMode:
            QGraphicsWidget.focusInEvent(self, event)
            
            
    def focusOutEvent(self, event):
        if not self.isDesignMode:
            QGraphicsWidget.focusOutEvent(self, event)
    
    
    def resizeEvent(self, event):
        self.adjustSize()


### Slots ###
    def onHorizontalLayout(self):
        self.layout().setOrientation(Qt.Horizontal)
        self.adjustSize()


    def onVerticalLayout(self):
        self.layout().setOrientation(Qt.Vertical)
        self.adjustSize()


    def onBreakLayout(self):
        self.scene().breakLayout(self)
        self.deleteLater()
        self.scene().removeItem(self)

