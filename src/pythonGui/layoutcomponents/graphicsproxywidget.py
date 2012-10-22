#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which a customwidget is
   component for the middle panel."""

__all__ = ["GraphicsProxyWidget"]


import displaycomponent
from displaywidget import DisplayWidget

import editableapplylatercomponent
import editablenoapplycomponent
from editablewidget import EditableWidget

from layoutcomponents.nodebase import NodeBase
from vacuumwidget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsProxyWidget(NodeBase, QGraphicsProxyWidget):


    def __init__(self, isEditable, widget, component=None, isStateToDisplay=False):
        super(GraphicsProxyWidget, self).__init__(isEditable)

        self.__component = component
        
        self.setWidget(widget)
        self.setFlags(QGraphicsItem.ItemIsFocusable)
        
        self.__contextMenu = None
        if self.__component:
            self._setupContextMenu(isStateToDisplay)


    def __del__(self):
        print "__del__", self
        NodeBase.__del__(self)


### private ###
    def _setupContextMenu(self, isStateToDisplay):
        # Populate context menu
        self.__contextMenu = QMenu()

        # Sub menu for widget type change
        self.__mChangeWidget = QMenu("Change widget")
        if isinstance(self.__component, displaycomponent.DisplayComponent):
            widgetAliases = DisplayWidget.getAliasesViaCategory(self, self.__component.widgetCategory)
            for i in range(len(widgetAliases)):
                acChangeWidget = self.__mChangeWidget.addAction(widgetAliases[i])
                acChangeWidget.triggered.connect(self.onChangeWidget)
            self.__contextMenu.addMenu(self.__mChangeWidget)
            
            # Only if state property is displayed...
            if isStateToDisplay:
                self.__contextMenu.addSeparator()

                # Sub menu for widget type change
                self.__mChangeVacuum = QMenu("Change vacuum widget")
                widgetAliases = VacuumWidget.getAliasesViaCategory(self, "State")
                for i in range(len(widgetAliases)):
                    acChangeVacuum = self.__mChangeVacuum.addAction(widgetAliases[i])
                    acChangeVacuum.triggered.connect(self.onChangeVacuumWidget)
                self.__contextMenu.addMenu(self.__mChangeVacuum)
        else:
            widgetAliases = EditableWidget.getAliasesViaCategory(self, self.__component.widgetCategory)
            for i in range(len(widgetAliases)):
                acChangeWidget = self.__mChangeWidget.addAction(widgetAliases[i])
                acChangeWidget.triggered.connect(self.onChangeWidget)
            self.__contextMenu.addMenu(self.__mChangeWidget)
        
        #self.__contextMenu.addSeparator()
        
        #text = "Remove widget"
        #self.__acRemove = QAction(QIcon(":no"), text, self)
        #self.__acRemove.setStatusTip(text)
        #self.__acRemove.setToolTip(text)
        #self.__acRemove.triggered.connect(self.onRemove)
        #self.__contextMenu.addAction(self.__acRemove)


### protected ###
    def paint(self, painter, option, widget):
        # Hack: self.parentItem() can only be a QGraphicsItemGroup
        if self.isSelected() and (self.parentItem() is None):
            pen = painter.pen()
            pen.setStyle(Qt.DashLine)
            painter.setPen(pen)
            #painter.setBrush(QColor(255,255,200))
            rect = self.boundingRect()
            painter.drawRect(rect)
        QGraphicsProxyWidget.paint(self, painter, option, widget)


    def mouseMoveEvent(self, event):
        #print "QGraphicsProxyWidget.mouseMoveEvent", self.isEditable
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseMoveEvent(self, event)
        else:
            QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        #print "QGraphicsProxyWidget.mousePressEvent", self.isEditable
        if (event.button() == Qt.RightButton):
            # If item is in GraphicsProxyWidgetContainer - forward event
            parentItem = self.parentItem()
            if parentItem:
                parentItem.mousePressEvent(event)
        
        if self.isEditable == True:
            self.setFlag(QGraphicsItem.ItemIsFocusable, True)
            QGraphicsProxyWidget.mousePressEvent(self, event)
        else:
            self.setFlag(QGraphicsItem.ItemIsFocusable, False)
            QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        #print "QGraphicsProxyWidget.mouseReleaseEvent", self.isEditable
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseReleaseEvent(self, event)
        else:
            QGraphicsItem.mouseReleaseEvent(self, event)


    def contextMenuEvent(self, event):
        if self.__contextMenu is None:
            return
        
        self.scene().clearSelection()
        self.setSelected(True)
        
        self.__contextMenu.exec_(event.screenPos())


### Slots ###
    def onChangeWidget(self):
        action = self.sender()
        # Change display or editable widget
        self.__component.changeWidget(action.text())
        self.adjustSize()


    def onChangeVacuumWidget(self):
        action = self.sender()
        # Change vacuum widget
        self.__component.changeToVacuumWidget(action.text())
        self.adjustSize()


    #def onRemove(self):
    #    print "onRemove"

