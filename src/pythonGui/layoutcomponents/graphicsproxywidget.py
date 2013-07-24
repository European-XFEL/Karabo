#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which a customwidget is
   component for the middle panel."""

__all__ = ["GraphicsProxyWidget"]


import displaycomponent
import displaywidget
from displaywidget import DisplayWidget

import editableapplylatercomponent
import editablenoapplycomponent
from editablewidget import EditableWidget

from layoutcomponents.nodebase import NodeBase
import vacuumwidget
from vacuumwidget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsProxyWidget(NodeBase, QGraphicsProxyWidget):


    def __init__(self, isDesignMode, widget, component=None, isStateToDisplay=False):
        super(GraphicsProxyWidget, self).__init__(isDesignMode)

        #self.setAcceptDrops(True)
        
        self.__component = component
        
        self.setWidget(widget)
        self.setFlags(QGraphicsItem.ItemIsFocusable)
        
        self.__contextMenu = None
        if self.__component:
            self._setupContextMenu(isStateToDisplay)


    def __del__(self):
        NodeBase.__del__(self)


    def destroy(self):
        if self.__component:
            self.__component.destroy()


    # Returns the component of this graphics proxy widget
    def _getComponent(self):
        return self.__component
    component = property(fget=_getComponent)


    # Returns the keys of the component
    def _getKeys(self):
        if self.__component:
            return self.__component.keys
        return None
    keys = property(fget=_getKeys)


### private ###
    def _setupContextMenu(self, isStateToDisplay):
        # Populate context menu
        self.__contextMenu = QMenu()

        # Sub menu for widget type change
        self.__mChangeWidget = QMenu("Change widget")
        if isinstance(self.__component, displaycomponent.DisplayComponent):
            widgetAliases = None
            widgetFactory = self.__component.widgetFactory
            if isinstance(widgetFactory, displaywidget.DisplayWidget):
                widgetAliases = DisplayWidget.getAliasesViaCategory(self, self.__component.widgetCategory)
            elif isinstance(widgetFactory, vacuumwidget.VacuumWidget):
                widgetAliases = VacuumWidget.getAliasesViaCategory(self, self.__component.widgetCategory)
            
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
        #print "QGraphicsProxyWidget.mouseMoveEvent", self.isDesignMode
        if self.isDesignMode == False:
            QGraphicsProxyWidget.mouseMoveEvent(self, event)
        else:
            QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        #print "QGraphicsProxyWidget.mousePressEvent", self.isDesignMode
        if self.isDesignMode == False:
            self.setFlag(QGraphicsItem.ItemIsFocusable, True)
            QGraphicsProxyWidget.mousePressEvent(self, event)
        else:
            self.setFlag(QGraphicsItem.ItemIsFocusable, False)
            QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        #print "QGraphicsProxyWidget.mouseReleaseEvent", self.isDesignMode
        if self.isDesignMode == False:
            QGraphicsProxyWidget.mouseReleaseEvent(self, event)
        else:
            QGraphicsItem.mouseReleaseEvent(self, event)


    #def dragEnterEvent(self, event):
    #    print "GraphicsProxyWidget.dragEnterEvent"
    #    QGraphicsProxyWidget.dragEnterEvent(self, event)


    #def dragMoveEvent(self, event):
    #    print "GraphicsProxyWidget.dragMoveEvent"
    #    QGraphicsProxyWidget.dragMoveEvent(self, event)


    #def dropEvent(self, event):
    #    print "GraphicsProxyWidget.dropEvent"
    #    QGraphicsProxyWidget.dropEvent(self, event)


    def contextMenuEvent(self, event):
        if self.__contextMenu is None:
            return
        if self.isDesignMode == False:
            QGraphicsProxyWidget.contextMenuEvent(self, event)
        else:
  
            self.scene().clearSelection()
            self.setSelected(True)

            self.__contextMenu.exec_(event.screenPos())


### Slots ###
    def onChangeWidget(self):
        if self.__component is None:
            return
        
        action = self.sender()
        # Change display or editable widget
        self.__component.changeWidget(action.text())
        self.adjustSize()
        
        parentWidget = self.parentWidget()
        if parentWidget:
            parentWidget.adjustSize()


    def onChangeVacuumWidget(self):
        if self.__component is None:
            return
        
        action = self.sender()
        # Change vacuum widget
        self.__component.changeToVacuumWidget(action.text())
        self.adjustSize()
        
        parentWidget = self.parentWidget()
        if parentWidget:
            parentWidget.adjustSize()


    #def onRemove(self):
    #    print "onRemove"

